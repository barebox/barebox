// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2016, Freescale Semiconductor, Inc.
//
// Best practice is to load OP-TEE early within prebootloader and
// run most of barebox in the normal world. OP-TEE, in at least
// some versions, relies on barebox however to setup the CAAM RNG.
// Similiarly, Linux, as of v6.1, can only initialize the CAAM
// via DECO, but this memory region may be reserved by OP-TEE for
// its own use. While the latter should be rather fixed by switching
// Linux to SH use, the former is a strong reason to poke the
// necessary bits here.

#define pr_fmt(fmt) "caam-pbl-init: " fmt

#include <io.h>
#include <dma.h>
#include <linux/printk.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>
#include <errno.h>
#include <pbl.h>
#include <string.h>
#include <soc/fsl/caam.h>
#include <asm/mmu.h>

#include "detect.h"
#include "regs.h"
#include "jr.h"
#include "desc.h"
#include "desc_constr.h"

#define rd_reg32_poll(addr, val, cond, tries) \
({ \
	int __tries = tries, __tmp; \
	__tmp = read_poll_timeout(rd_reg32, val, (cond) || __tries--, \
			0, (addr)); \
	__tries ? __tmp : -ETIMEDOUT; \
})

static struct caam_ctrl *caam;

struct jr_data_st {
	u8 inrings[16];
	u8 outrings[16];
	u32 desc[3 * MAX_CAAM_DESCSIZE / sizeof(u32)];
} __aligned(8);

static struct jr_data_st *g_jrdata;

static void dump_error(void)
{
	struct rng4tst __iomem *r4tst = &caam->r4tst[0];
	int i;

	pr_debug("Dump CAAM Error\n");
	pr_debug("MCFGR    0x%08x\n", rd_reg32(&caam->mcr));
	pr_debug("FAR      0x%08x\n", rd_reg32(&caam->perfmon.faultaddr));
	pr_debug("FAMR     0x%08x\n", rd_reg32(&caam->perfmon.faultliodn));
	pr_debug("FADR     0x%08x\n", rd_reg32(&caam->perfmon.faultdetail));
	pr_debug("CSTA     0x%08x\n", rd_reg32(&caam->perfmon.status));
	pr_debug("RTMCTL   0x%08x\n", rd_reg32(&r4tst->rtmctl));
	pr_debug("RTSTATUS 0x%08x\n", rd_reg32(&r4tst->rtstatus));
	pr_debug("RDSTA    0x%08x\n", rd_reg32(&r4tst->rdsta));

	for (i = 0; i < desc_len(g_jrdata->desc); i++)
		pr_debug("desc[%2d] 0x%08x\n", i, g_jrdata->desc[i]);
}

#define CAAM_JUMP_OFFSET(x) ((x) & JUMP_OFFSET_MASK)

/* Descriptors to instantiate SH0, SH1, load the keys */
static const u32 rng_inst_sh0_desc[] = {
	/* Header, don't setup the size */
	CMD_DESC_HDR | IMMEDIATE,
	/* Operation instantiation (sh0) */
	CMD_OPERATION | OP_ALG_ALGSEL_RNG | OP_ALG_TYPE_CLASS1 | OP_ALG_AAI_RNG4_SH_0
		| OP_ALG_AS_INIT | OP_ALG_PR_ON,
};

static const u32 rng_inst_sh1_desc[] = {
	/* wait for done - Jump to next entry */
	CMD_JUMP | CLASS_1 | JUMP_TEST_ALL | CAAM_JUMP_OFFSET(1),
	/* Clear written register (write 1) */
	CMD_LOAD | LDST_IMM | LDST_SRCDST_WORD_CLRW | sizeof(u32),
	0x00000001,
	/* Operation instantiation (sh1) */
	CMD_OPERATION | OP_ALG_ALGSEL_RNG | OP_ALG_TYPE_CLASS1 | OP_ALG_AAI_RNG4_SH_1
		| OP_ALG_AS_INIT | OP_ALG_PR_ON,
};

static const u32 rng_inst_load_keys[] = {
	/* wait for done - Jump to next entry */
	CMD_JUMP | CLASS_1 | JUMP_TEST_ALL | CAAM_JUMP_OFFSET(1),
	/* Clear written register (write 1) */
	CMD_LOAD | LDST_IMM | LDST_SRCDST_WORD_CLRW | sizeof(u32),
	0x00000001,
	/* Generate the Key */
	CMD_OPERATION | OP_ALG_ALGSEL_RNG | OP_ALG_TYPE_CLASS1 | OP_ALG_AAI_RNG4_SK,
};

static int do_job(struct caam_job_ring __iomem *jr, u32 *desc, u32 *ecode)
{
	phys_addr_t p_desc = cpu_to_caam_dma((dma_addr_t)desc);
	u32 status;
	int ret = 0;

	if (rd_reg32(&jr->inpring_avail) == 0)
		return -EBUSY;

	jr_inpentry_set(g_jrdata->inrings, 0, p_desc);

	barrier();

	/* Inform HW that a new JR is available */
	wr_reg32(&jr->inpring_jobadd, 1);
	while (rd_reg32(&jr->outring_used) == 0)
		;

	if (p_desc == jr_outentry_desc(g_jrdata->outrings, 0)) {
		status = caam32_to_cpu(jr_outentry_jrstatus(g_jrdata->outrings, 0));
		if (ecode)
			*ecode = status;
	} else {
		dump_error();
		ret = -ENODATA;
	}

	/* Acknowledge interrupt */
	setbits_le32(&jr->jrintstatus, JRINT_JR_INT);
	/* Remove the JR from the output list even if no JR caller found */
	wr_reg32(&jr->outring_rmvd, 1);

	return ret;
}

static int do_cfg_jrqueue(struct caam_job_ring __iomem *jr)
{
	u32 value = 0;
	phys_addr_t ip_base;
	phys_addr_t op_base;

	/* Configure the HW Job Rings */
	ip_base = cpu_to_caam_dma((dma_addr_t)g_jrdata->inrings);
	op_base = cpu_to_caam_dma((dma_addr_t)g_jrdata->outrings);

	wr_reg64(&jr->inpring_base, ip_base);
	wr_reg32(&jr->inpring_size, 1);

	wr_reg64(&jr->outring_base, op_base);
	wr_reg32(&jr->outring_size, 1);

	setbits_le32(&jr->jrintstatus, JRINT_JR_INT);

	/*
	 * Configure interrupts but disable it:
	 * Optimization to generate an interrupt either when there are
	 * half of the job done or when there is a job done and
	 * 10 clock cycles elapse without new job complete
	 */
	value = 10 << JRCFG_ICTT_SHIFT;
	value |= 1 << JRCFG_ICDCT_SHIFT;
	value |= JRCFG_ICEN;
	value |= JRCFG_IMSK;
	wr_reg32(&jr->rconfig_lo, value);

	/* Enable deco watchdog */
	setbits_le32(&caam->mcr, MCFGR_WDENABLE);

	return 0;
}

static void do_clear_rng_error(struct rng4tst __iomem *r4tst)
{
	if (rd_reg32(&r4tst->rtmctl) & (RTMCTL_ERR | RTMCTL_FCT_FAIL)) {
		setbits_le32(&r4tst->rtmctl, RTMCTL_ERR);
		(void)rd_reg32(&r4tst->rtmctl);
	}
}

static void do_inst_desc(u32 *desc, u32 status)
{
	u32 *pdesc = desc;
	u8  desc_len;
	bool add_sh0   = false;
	bool add_sh1   = false;
	bool load_keys = false;

	/*
	 * Modify the the descriptor to remove if necessary:
	 *  - The key loading
	 *  - One of the SH already instantiated
	 */
	desc_len = sizeof(rng_inst_sh0_desc);
	if ((status & RDSTA_IF0) != RDSTA_IF0)
		add_sh0 = true;

	if ((status & RDSTA_IF1) != RDSTA_IF1) {
		add_sh1 = true;
		if (add_sh0)
			desc_len += sizeof(rng_inst_sh0_desc);
	}

	if ((status & RDSTA_SKVN) != RDSTA_SKVN) {
		load_keys = true;
		desc_len += sizeof(rng_inst_load_keys);
	}

	/* Copy the SH0 descriptor anyway */
	memcpy(pdesc, rng_inst_sh0_desc, sizeof(rng_inst_sh0_desc));
	pdesc += ARRAY_SIZE(rng_inst_sh0_desc);

	if (load_keys) {
		pr_debug("RNG - Load keys\n");
		memcpy(pdesc, rng_inst_load_keys, sizeof(rng_inst_load_keys));
		pdesc += ARRAY_SIZE(rng_inst_load_keys);
	}

	if (add_sh1) {
		if (add_sh0) {
			pr_debug("RNG - Instantiation of SH0 and SH1\n");
			/* Add the sh1 descriptor */
			memcpy(pdesc, rng_inst_sh1_desc,
				sizeof(rng_inst_sh1_desc));
		} else {
			pr_debug("RNG - Instantiation of SH1 only\n");
			/* Modify the SH0 descriptor to instantiate only SH1 */
			desc[1] &= ~OP_ALG_AAI_RNG4_SH_MASK;
			desc[1] |= OP_ALG_AAI_RNG4_SH_1;
		}
	}

	/* Setup the descriptor size */
	desc[0] &= ~HDR_DESCLEN_SHR_MASK;
	desc[0] |= desc_len & HDR_DESCLEN_SHR_MASK;
}

static void kick_trng(struct rng4tst __iomem *r4tst, u32 ent_delay)
{
	u32 samples  = 512; /* number of bits to generate and test */
	u32 mono_min = 195;
	u32 mono_max = 317;
	u32 mono_range  = mono_max - mono_min;
	u32 poker_min = 1031;
	u32 poker_max = 1600;
	u32 poker_range = poker_max - poker_min + 1;
	u32 retries    = 2;
	u32 lrun_max   = 32;
	s32 run_1_min   = 27;
	s32 run_1_max   = 107;
	s32 run_1_range = run_1_max - run_1_min;
	s32 run_2_min   = 7;
	s32 run_2_max   = 62;
	s32 run_2_range = run_2_max - run_2_min;
	s32 run_3_min   = 0;
	s32 run_3_max   = 39;
	s32 run_3_range = run_3_max - run_3_min;
	s32 run_4_min   = -1;
	s32 run_4_max   = 26;
	s32 run_4_range = run_4_max - run_4_min;
	s32 run_5_min   = -1;
	s32 run_5_max   = 18;
	s32 run_5_range = run_5_max - run_5_min;
	s32 run_6_min   = -1;
	s32 run_6_max   = 17;
	s32 run_6_range = run_6_max - run_6_min;
	u32 val;

	/* Put RNG in program mode */
	/* Setting both RTMCTL:PRGM and RTMCTL:TRNG_ACC causes TRNG to
	 * properly invalidate the entropy in the entropy register and
	 * force re-generation.
	 */
	setbits_le32(&r4tst->rtmctl, RTMCTL_PRGM | RTMCTL_ACC);

	/* Configure the RNG Entropy Delay
	 * Performance-wise, it does not make sense to
	 * set the delay to a value that is lower
	 * than the last one that worked (i.e. the state handles
	 * were instantiated properly. Thus, instead of wasting
	 * time trying to set the values controlling the sample
	 * frequency, the function simply returns.
	 */
	val = rd_reg32(&r4tst->rtsdctl);
	if (ent_delay < FIELD_GET(RTSDCTL_ENT_DLY_MASK, val)) {
		/* Put RNG4 into run mode */
		clrbits_le32(&r4tst->rtmctl, RTMCTL_PRGM | RTMCTL_ACC);
		return;
	}

	val = (ent_delay << RTSDCTL_ENT_DLY_SHIFT) | samples;
	wr_reg32(&r4tst->rtsdctl, val);

	/* min. freq. count, equal to 1/2 of the entropy sample length */
	wr_reg32(&r4tst->rtfrqmin, ent_delay >> 1);

	/* max. freq. count, equal to 32 times the entropy sample length */
	wr_reg32(&r4tst->rtfrqmax, ent_delay << 5);

	wr_reg32(&r4tst->rtscmisc, (retries << 16) | lrun_max);
	wr_reg32(&r4tst->rtpkrmax, poker_max);
	wr_reg32(&r4tst->rtpkrrng, poker_range);
	wr_reg32(&r4tst->rtscml, (mono_range << 16) | mono_max);
	wr_reg32(&r4tst->rtscr1l, (run_1_range << 16) | run_1_max);
	wr_reg32(&r4tst->rtscr2l, (run_2_range << 16) | run_2_max);
	wr_reg32(&r4tst->rtscr3l, (run_3_range << 16) | run_3_max);
	wr_reg32(&r4tst->rtscr4l, (run_4_range << 16) | run_4_max);
	wr_reg32(&r4tst->rtscr5l, (run_5_range << 16) | run_5_max);
	wr_reg32(&r4tst->rtscr6pl, (run_6_range << 16) | run_6_max);

	/*
	 * select raw sampling in both entropy shifter
	 * and statistical checker; ; put RNG4 into run mode
	 */
	clrsetbits_32(&r4tst->rtmctl, RTMCTL_PRGM | RTMCTL_ACC | RTMCTL_SAMP_MODE_MASK,
		      RTMCTL_SAMP_MODE_RAW_ES_SC);

	/* Clear the ERR bit in RTMCTL if set. The TRNG error can occur when the
	 * RNG clock is not within 1/2x to 8x the system clock.
	 * This error is possible if ROM code does not initialize the system PLLs
	 * immediately after PoR.
	 */
	/* setbits_le32(&r4tst->rtmctl, RTMCTL_ERR); */
}

static int do_instantiation(struct caam_job_ring __iomem *jr,
			    struct rng4tst __iomem *r4tst)
{
	struct caam_perfmon __iomem *perfmon = &caam->perfmon;
	int ret;
	u32 cha_vid_ls, rng_vid;
	u32 ent_delay;
	u32 status;

	if (!g_jrdata) {
		pr_err("descriptor allocation failed\n");
		return -ENODEV;
	}

	cha_vid_ls = rd_reg32(&perfmon->cha_id_ls);

	/*
	 * If SEC has RNG version >= 4 and RNG state handle has not been
	 * already instantiated, do RNG instantiation
	 */
	rng_vid = FIELD_GET(CHAVID_LS_RNGVID_MASK, cha_vid_ls);
	if (rng_vid < 4) {
		pr_info("RNG (VID=%u) already instantiated.\n", rng_vid);
		return 0;
	}

	ent_delay = RTSDCTL_ENT_DLY_MIN;

	do {
		/* Read the CAAM RNG status */
		status = rd_reg32(&r4tst->rdsta);

		if ((status & RDSTA_IF0) != RDSTA_IF0) {
			/* Configure the RNG entropy delay */
			kick_trng(r4tst, ent_delay);
			ent_delay += 400;
		}

		do_clear_rng_error(r4tst);

		if ((status & (RDSTA_IF0 | RDSTA_IF1)) != (RDSTA_IF0 | RDSTA_IF1)) {
			do_inst_desc(g_jrdata->desc, status);

			ret = do_job(jr, g_jrdata->desc, NULL);
			if (ret < 0) {
				pr_err("RNG Instantiation failed\n");
				goto end_instantation;
			}
		} else {
			ret = 0;
			pr_debug("RNG instantiation done (%d)\n", ent_delay);
			goto end_instantation;
		}
	} while (ent_delay < RTSDCTL_ENT_DLY_MAX);

	pr_err("RNG Instantation Failure - Entropy delay (%d)\n", ent_delay);
	ret = -ETIMEDOUT;

end_instantation:
	return ret;
}

static int jr_reset(struct caam_job_ring __iomem *jr)
{
	int ret;
	u32 val;

	/* Mask interrupts to poll for reset completion status */
	setbits_le32(&jr->rconfig_lo, JRCFG_IMSK);

	/* Initiate flush of all pending jobs (required prior to reset) */
	wr_reg32(&jr->jrcommand, JRCR_RESET);

	ret = rd_reg32_poll(&jr->jrintstatus, val,
			    val != JRINT_ERR_HALT_INPROGRESS, 10000);

	if (ret || val != JRINT_ERR_HALT_COMPLETE) {
		pr_err("failed to flush job ring\n");
		return ret ?: -EIO;
	}

	/* Initiate reset by setting reset bit a second time */
	wr_reg32(&jr->jrcommand, JRCR_RESET);

	ret = rd_reg32_poll(&jr->jrcommand, val, !(val & JRCR_RESET), 100);
	if (ret) {
		pr_err("failed to reset job ring\n");
		return ret;
	}

	return 0;
}


static int rng_init(struct caam_job_ring __iomem *jr,
		    struct rng4tst __iomem *r4tst)
{
	int  ret;

	ret = jr_reset(jr);
	if (ret)
		return ret;

	ret = do_instantiation(jr, r4tst);
	if (ret)
		return ret;

	jr_reset(jr);
	return 0;
}

bool caam_little_end;
bool caam_imx;
size_t caam_ptr_sz;

int early_caam_init(struct caam_ctrl __iomem *_caam, bool is_imx)
{
	static struct jr_data_st pbl_jrdata;
	struct caam_job_ring __iomem *jr;
	struct rng4tst __iomem *r4tst;
	u32 temp_reg;
	int ret;

	caam = _caam;
	caam_imx = is_imx;
	caam_little_end = !caam_is_big_endian(caam);
	caam_ptr_sz = caam_is_64bit(caam) ? sizeof(u64) : sizeof(u32);

	/*
	 * PBL will only enable MMU right before unpacking, so all memory
	 * is uncached and thus coherent here
	 */
	if (IN_PBL)
		g_jrdata = &pbl_jrdata;
	else
		g_jrdata = dma_alloc_coherent(DMA_DEVICE_BROKEN, sizeof(*g_jrdata),
					      DMA_ADDRESS_BROKEN);

	jr = IOMEM(caam) + 0x1000;
	r4tst = &caam->r4tst[0];

	pr_debug("Detected %zu-bit %s-endian %sCAAM\n", caam_ptr_sz * 8,
		 caam_little_end ? "little" : "big", caam_imx ? "i.MX " : "");

	/* reset the CAAM */
	temp_reg = rd_reg32(&caam->mcr) | MCFGR_DMA_RESET | MCFGR_SWRESET;
	wr_reg32(&caam->mcr, temp_reg);

	while (rd_reg32(&caam->mcr) & MCFGR_DMA_RESET)
		;

	jr_reset(jr);

	ret = do_cfg_jrqueue(jr);
	if (ret) {
		pr_err("job ring init failed\n");
		return ret;
	}

	/* Check if the RNG is already instantiated */
	temp_reg = rd_reg32(&r4tst->rdsta);
	if (temp_reg == (RDSTA_IF0 | RDSTA_IF1 | RDSTA_SKVN)) {
		pr_notice("RNG already instantiated 0x%x\n", temp_reg);
		return 0;
	}

	return rng_init(jr, r4tst);
}
