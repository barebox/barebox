/*
 * CAAM control-plane driver backend
 * Controller-level driver, kernel property detection, initialization
 *
 * Copyright 2008-2012 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <clock.h>
#include <driver.h>
#include <hab.h>
#include <init.h>
#include <linux/barebox-wrapper.h>
#include <linux/spinlock.h>
#include <linux/clk.h>

#include "regs.h"
#include "intern.h"
#include "jr.h"
#include "desc_constr.h"
#include "error.h"
#include "ctrl.h"
#include "rng_self_test.h"

bool caam_little_end;
EXPORT_SYMBOL(caam_little_end);

/*
 * Descriptor to instantiate RNG State Handle 0 in normal mode and
 * load the JDKEK, TDKEK and TDSK registers
 */
static void build_instantiation_desc(u32 *desc, int handle, int do_sk)
{
	u32 *jump_cmd, op_flags;

	init_job_desc(desc, 0);

	op_flags = OP_TYPE_CLASS1_ALG | OP_ALG_ALGSEL_RNG |
			(handle << OP_ALG_AAI_SHIFT) | OP_ALG_AS_INIT;

	/* INIT RNG in non-test mode */
	append_operation(desc, op_flags);

	if (!handle && do_sk) {
		/*
		 * For SH0, Secure Keys must be generated as well
		 */

		/* wait for done */
		jump_cmd = append_jump(desc, JUMP_CLASS_CLASS1);
		set_jump_tgt_here(desc, jump_cmd);

		/*
		 * load 1 to clear written reg:
		 * resets the done interrrupt and returns the RNG to idle.
		 */
		append_load_imm_u32(desc, 1, LDST_SRCDST_WORD_CLRW);

		/* Initialize State Handle  */
		append_operation(desc, OP_TYPE_CLASS1_ALG | OP_ALG_ALGSEL_RNG |
				 OP_ALG_AAI_RNG4_SK);
	}

	append_jump(desc, JUMP_CLASS_CLASS1 | JUMP_TYPE_HALT);
}

/*
 * run_descriptor_deco0 - runs a descriptor on DECO0, under direct control of
 *			  the software (no JR/QI used).
 * @ctrldev - pointer to device
 * @status - descriptor status, after being run
 *
 * Return: - 0 if no error occurred
 *	   - -ENODEV if the DECO couldn't be acquired
 *	   - -EAGAIN if an error occurred while executing the descriptor
 */
static inline int run_descriptor_deco0(struct device_d *ctrldev, u32 *desc,
					u32 *status)
{
	struct caam_drv_private *ctrlpriv = ctrldev->priv;
	struct caam_ctrl __iomem *ctrl;
	struct caam_deco __iomem *deco;
	u32 deco_dbg_reg, flags;
	uint64_t start;
	int i;

	ctrl = ctrlpriv->ctrl;
	deco = ctrlpriv->deco;

	if (ctrlpriv->virt_en == 1) {
		clrsetbits_32(&ctrl->deco_rsr, 0, DECORSR_JR0);

		start = get_time_ns();
		while (!(rd_reg32(&ctrl->deco_rsr) & DECORSR_VALID)) {
			if (is_timeout(start, 100 * MSECOND)) {
				dev_err(ctrldev, "DECO timed out\n");
				return -ETIMEDOUT;
			}
		}
	}

	clrsetbits_32(&ctrl->deco_rq, 0, DECORR_RQD0ENABLE);

	start = get_time_ns();
	while (!(rd_reg32(&ctrl->deco_rq) & DECORR_DEN0)) {
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(ctrldev, "failed to acquire DECO 0\n");
			clrsetbits_32(&ctrl->deco_rq, DECORR_RQD0ENABLE, 0);
			return -ETIMEDOUT;
		}
	}

	for (i = 0; i < desc_len(desc); i++)
		wr_reg32(&deco->descbuf[i], caam32_to_cpu(*(desc + i)));

	flags = DECO_JQCR_WHL;
	/*
	 * If the descriptor length is longer than 4 words, then the
	 * FOUR bit in JRCTRL register must be set.
	 */
	if (desc_len(desc) >= 4)
		flags |= DECO_JQCR_FOUR;

	/* Instruct the DECO to execute it */
	clrsetbits_32(&deco->jr_ctl_hi, 0, flags);

	start = get_time_ns();
	while ((deco_dbg_reg = rd_reg32(&deco->desc_dbg)) &
		 DESC_DBG_DECO_STAT_VALID) {
		/*
		 * If an error occured in the descriptor, then
		 * the DECO status field will be set to 0x0D
		 */
		if ((deco_dbg_reg & DESC_DBG_DECO_STAT_MASK) ==
		    DESC_DBG_DECO_STAT_HOST_ERR)
			break;
	}

	*status = rd_reg32(&deco->op_status_hi) &
		  DECO_OP_STATUS_HI_ERR_MASK;

	if (ctrlpriv->virt_en == 1)
		clrsetbits_32(&ctrl->deco_rsr, DECORSR_JR0, 0);

	/* Mark the DECO as free */
	clrsetbits_32(&ctrl->deco_rq, DECORR_RQD0ENABLE, 0);

	if (is_timeout(start, 100 * MSECOND))
		return -EAGAIN;

	return 0;
}

/*
 * instantiate_rng - builds and executes a descriptor on DECO0,
 *		     which initializes the RNG block.
 * @ctrldev - pointer to device
 * @state_handle_mask - bitmask containing the instantiation status
 *			for the RNG4 state handles which exist in
 *			the RNG4 block: 1 if it's been instantiated
 *			by an external entry, 0 otherwise.
 * @gen_sk  - generate data to be loaded into the JDKEK, TDKEK and TDSK;
 *	      Caution: this can be done only once; if the keys need to be
 *	      regenerated, a POR is required
 *
 * Return: - 0 if no error occurred
 *	   - -ENOMEM if there isn't enough memory to allocate the descriptor
 *	   - -ENODEV if DECO0 couldn't be acquired
 *	   - -EAGAIN if an error occurred when executing the descriptor
 *	      f.i. there was a RNG hardware error due to not "good enough"
 *	      entropy being aquired.
 */
static int instantiate_rng(struct device_d *ctrldev, int state_handle_mask,
			   int gen_sk)
{
	struct caam_drv_private *ctrlpriv = ctrldev->priv;
	struct caam_ctrl __iomem *ctrl;
	u32 *desc, status = 0, rdsta_val;
	int ret = 0, sh_idx;

	ctrl = (struct caam_ctrl __iomem *)ctrlpriv->ctrl;
	desc = xzalloc(CAAM_CMD_SZ * 7);

	for (sh_idx = 0; sh_idx < RNG4_MAX_HANDLES; sh_idx++) {
		/*
		 * If the corresponding bit is set, this state handle
		 * was initialized by somebody else, so it's left alone.
		 */
		if ((1 << sh_idx) & state_handle_mask)
			continue;

		/* Create the descriptor for instantiating RNG State Handle */
		build_instantiation_desc(desc, sh_idx, gen_sk);

		/* Try to run it through DECO0 */
		ret = run_descriptor_deco0(ctrldev, desc, &status);

		/*
		 * If ret is not 0, or descriptor status is not 0, then
		 * something went wrong. No need to try the next state
		 * handle (if available), bail out here.
		 * Also, if for some reason, the State Handle didn't get
		 * instantiated although the descriptor has finished
		 * without any error (HW optimizations for later
		 * CAAM eras), then try again.
		 */
		if (ret)
			break;

		rdsta_val = rd_reg32(&ctrl->r4tst[0].rdsta) & RDSTA_IFMASK;
		if ((status && status != JRSTA_SSRC_JUMP_HALT_CC) ||
		    !(rdsta_val & (1 << sh_idx))) {
			ret = -EAGAIN;
			break;
		}
		dev_info(ctrldev, "Instantiated RNG4 SH%d\n", sh_idx);
		/* Clear the contents before recreating the descriptor */
		memset(desc, 0x00, CAAM_CMD_SZ * 7);
	}

	return ret;
}

static void caam_remove(struct device_d *dev)
{
	struct caam_drv_private *ctrlpriv = dev->priv;

	/* shut clocks off before finalizing shutdown */
	clk_disable(ctrlpriv->caam_ipg);
	if (ctrlpriv->caam_mem)
		clk_disable(ctrlpriv->caam_mem);
	clk_disable(ctrlpriv->caam_aclk);
	if (ctrlpriv->caam_emi_slow)
		clk_disable(ctrlpriv->caam_emi_slow);
}

/*
 * kick_trng - sets the various parameters for enabling the initialization
 *	       of the RNG4 block in CAAM
 * @pdev - pointer to the platform device
 * @ent_delay - Defines the length (in system clocks) of each entropy sample.
 */
static void kick_trng(struct device_d *ctrldev, int ent_delay)
{
	struct caam_drv_private *ctrlpriv = ctrldev->priv;
	struct caam_ctrl __iomem *ctrl;
	struct rng4tst __iomem *r4tst;
	u32 val;

	ctrl = (struct caam_ctrl __iomem *)ctrlpriv->ctrl;
	r4tst = &ctrl->r4tst[0];

	/* put RNG4 into program mode */
	clrsetbits_32(&r4tst->rtmctl, 0, RTMCTL_PRGM);

	/*
	 * Performance-wise, it does not make sense to
	 * set the delay to a value that is lower
	 * than the last one that worked (i.e. the state handles
	 * were instantiated properly. Thus, instead of wasting
	 * time trying to set the values controlling the sample
	 * frequency, the function simply returns.
	 */
	val = (rd_reg32(&r4tst->rtsdctl) & RTSDCTL_ENT_DLY_MASK)
	      >> RTSDCTL_ENT_DLY_SHIFT;
	if (ent_delay <= val)
		goto start_rng;

	val = rd_reg32(&r4tst->rtsdctl);
	val = (val & ~RTSDCTL_ENT_DLY_MASK) |
	      (ent_delay << RTSDCTL_ENT_DLY_SHIFT);
	wr_reg32(&r4tst->rtsdctl, val);
	/* min. freq. count, equal to 1/4 of the entropy sample length */
	wr_reg32(&r4tst->rtfrqmin, ent_delay >> 2);
	/* disable maximum frequency count */
	wr_reg32(&r4tst->rtfrqmax, RTFRQMAX_DISABLE);
	/* read the control register */
	val = rd_reg32(&r4tst->rtmctl);
start_rng:
	/*
	 * select raw sampling in both entropy shifter
	 * and statistical checker; ; put RNG4 into run mode
	 */
	clrsetbits_32(&r4tst->rtmctl, RTMCTL_PRGM, RTMCTL_SAMP_MODE_RAW_ES_SC);
}

static int caam_get_era_from_hw(struct caam_ctrl __iomem *ctrl)
{
	static const struct {
		u16 ip_id;
		u8 maj_rev;
		u8 era;
	} id[] = {
		{0x0A10, 1, 1},
		{0x0A10, 2, 2},
		{0x0A12, 1, 3},
		{0x0A14, 1, 3},
		{0x0A14, 2, 4},
		{0x0A16, 1, 4},
		{0x0A10, 3, 4},
		{0x0A11, 1, 4},
		{0x0A18, 1, 4},
		{0x0A11, 2, 5},
		{0x0A12, 2, 5},
		{0x0A13, 1, 5},
		{0x0A1C, 1, 5}
	};
	u32 ccbvid, id_ms;
	u8 maj_rev, era;
	u16 ip_id;
	int i;

	ccbvid = rd_reg32(&ctrl->perfmon.ccb_id);
	era = (ccbvid & CCBVID_ERA_MASK) >> CCBVID_ERA_SHIFT;
	if (era)	/* This is '0' prior to CAAM ERA-6 */
		return era;

	id_ms = rd_reg32(&ctrl->perfmon.caam_id_ms);
	ip_id = (id_ms & SECVID_MS_IPID_MASK) >> SECVID_MS_IPID_SHIFT;
	maj_rev = (id_ms & SECVID_MS_MAJ_REV_MASK) >> SECVID_MS_MAJ_REV_SHIFT;

	for (i = 0; i < ARRAY_SIZE(id); i++)
		if (id[i].ip_id == ip_id && id[i].maj_rev == maj_rev)
			return id[i].era;

	return -ENOTSUPP;
}

/**
 * caam_get_era() - Return the ERA of the SEC on SoC, based
 * on "sec-era" optional property in the DTS. This property is updated
 * by u-boot.
 * In case this property is not passed an attempt to retrieve the CAAM
 * era via register reads will be made.
 **/
static int caam_get_era(struct caam_ctrl __iomem *ctrl)
{
	struct device_node *caam_node;
	int ret;
	u32 prop;

	caam_node = of_find_compatible_node(NULL, NULL, "fsl,sec-v4.0");
	ret = of_property_read_u32(caam_node, "fsl,sec-era", &prop);

	if (!ret)
		return prop;
	else
		return caam_get_era_from_hw(ctrl);
}

/* Probe routine for CAAM top (controller) level */
static int caam_probe(struct device_d *dev)
{
	int ret, ring, rspec, gen_sk, ent_delay = RTSDCTL_ENT_DLY_MIN;
	u64 caam_id;
	struct device_node *nprop, *np;
	struct caam_ctrl __iomem *ctrl;
	struct caam_drv_private *ctrlpriv;
	u32 scfgr, comp_params;
	u32 cha_vid_ls;
	int pg_size;
	int BLOCK_OFFSET = 0;

	ctrlpriv = xzalloc(sizeof(struct caam_drv_private));

	dev->priv = ctrlpriv;
	ctrlpriv->pdev = dev;
	nprop = dev->device_node;

	ctrlpriv->caam_ipg = clk_get(dev, "ipg");
	if (IS_ERR(ctrlpriv->caam_ipg)) {
		ret = PTR_ERR(ctrlpriv->caam_ipg);
		dev_err(dev, "can't identify CAAM ipg clk: %d\n", ret);
		return -ENODEV;
	}

	if (!of_machine_is_compatible("fsl,imx7d") &&
	    !of_machine_is_compatible("fsl,imx7s")) {
		ctrlpriv->caam_mem = clk_get(dev, "mem");
		if (IS_ERR(ctrlpriv->caam_mem)) {
			ret = PTR_ERR(ctrlpriv->caam_mem);
			dev_err(dev,
				"can't identify CAAM mem clk: %d\n", ret);
			return -ENODEV;
		}
	}

	ctrlpriv->caam_aclk = clk_get(dev, "aclk");
	if (IS_ERR(ctrlpriv->caam_aclk)) {
		ret = PTR_ERR(ctrlpriv->caam_aclk);
		dev_err(dev,
			"can't identify CAAM aclk clk: %d\n", ret);
		return -ENODEV;
	}

	if (!of_machine_is_compatible("fsl,imx6ul") &&
	    !of_machine_is_compatible("fsl,imx7d") &&
	    !of_machine_is_compatible("fsl,imx7s")) {
		ctrlpriv->caam_emi_slow = clk_get(dev, "emi_slow");
		if (IS_ERR(ctrlpriv->caam_emi_slow)) {
			ret = PTR_ERR(ctrlpriv->caam_emi_slow);
			dev_err(dev,
				"can't identify CAAM emi slow clk: %d\n", ret);
			return -ENODEV;
		}
	}

	ret = clk_enable(ctrlpriv->caam_ipg);
	if (ret < 0) {
		dev_err(dev, "can't enable CAAM ipg clock: %d\n", ret);
		return -ENODEV;
	}

	if (ctrlpriv->caam_mem) {
		ret = clk_enable(ctrlpriv->caam_mem);
		if (ret < 0) {
			dev_err(dev, "can't enable CAAM secure mem clock: %d\n",
				ret);
			return -ENODEV;
		}
	}

	ret = clk_enable(ctrlpriv->caam_aclk);
	if (ret < 0) {
		dev_err(dev, "can't enable CAAM aclk clock: %d\n", ret);
		return -ENODEV;
	}

	if (ctrlpriv->caam_emi_slow) {
		ret = clk_enable(ctrlpriv->caam_emi_slow);
		if (ret < 0) {
			dev_err(dev, "can't enable CAAM emi slow clock: %d\n",
				ret);
			return -ENODEV;
		}
	}

	/* Get configuration properties from device tree */
	/* First, get register page */
	ctrl = dev_request_mem_region(dev, 0);
	if (ctrl == NULL) {
		dev_err(dev, "caam: of_iomap() failed\n");
		return -ENOMEM;
	}

	caam_little_end = !(bool)(rd_reg32(&ctrl->perfmon.status) &
				  (CSTA_PLEND | CSTA_ALT_PLEND));

	/* Finding the page size for using the CTPR_MS register */
	comp_params = rd_reg32(&ctrl->perfmon.comp_parms_ms);
	pg_size = (comp_params & CTPR_MS_PG_SZ_MASK) >> CTPR_MS_PG_SZ_SHIFT;

	/* Allocating the BLOCK_OFFSET based on the supported page size on
	 * the platform
	 */
	if (pg_size == 0)
		BLOCK_OFFSET = PG_SIZE_4K;
	else
		BLOCK_OFFSET = PG_SIZE_64K;

	ctrlpriv->ctrl = (struct caam_ctrl __iomem __force *)ctrl;
	ctrlpriv->assure = (struct caam_assurance __iomem __force *)
			   ((__force uint8_t *)ctrl +
			    BLOCK_OFFSET * ASSURE_BLOCK_NUMBER);
	ctrlpriv->deco = (struct caam_deco __iomem __force *)
			 ((__force uint8_t *)ctrl +
			  BLOCK_OFFSET * DECO_BLOCK_NUMBER);

	/*
	 * Enable DECO watchdogs and, if this is a PHYS_ADDR_T_64BIT kernel,
	 * long pointers in master configuration register
	 */
	clrsetbits_32(&ctrl->mcr, MCFGR_AWCACHE_MASK | MCFGR_ARCACHE_MASK,
			MCFGR_AWCACHE_CACH | MCFGR_ARCACHE_MASK |
			MCFGR_WDENABLE | (sizeof(dma_addr_t) == sizeof(u64) ?
					  MCFGR_LONG_PTR : 0));

	/*
	 *  Read the Compile Time paramters and SCFGR to determine
	 * if Virtualization is enabled for this platform
	 */
	scfgr = rd_reg32(&ctrl->scfgr);

	ctrlpriv->virt_en = 0;
	if (comp_params & CTPR_MS_VIRT_EN_INCL) {
		/* VIRT_EN_INCL = 1 & VIRT_EN_POR = 1 or
		 * VIRT_EN_INCL = 1 & VIRT_EN_POR = 0 & SCFGR_VIRT_EN = 1
		 */
		if ((comp_params & CTPR_MS_VIRT_EN_POR) ||
		    (!(comp_params & CTPR_MS_VIRT_EN_POR) &&
		     (scfgr & SCFGR_VIRT_EN)))
			ctrlpriv->virt_en = 1;
	} else {
		/* VIRT_EN_INCL = 0 && VIRT_EN_POR_VALUE = 1 */
		if (comp_params & CTPR_MS_VIRT_EN_POR)
			ctrlpriv->virt_en = 1;
	}

	if (ctrlpriv->virt_en == 1)
		clrsetbits_32(&ctrl->jrstart, 0, JRSTART_JR0_START |
			      JRSTART_JR1_START | JRSTART_JR2_START |
			      JRSTART_JR3_START);

	/*
	 * ERRATA:  mx6 devices have an issue wherein AXI bus transactions
	 * may not occur in the correct order. This isn't a problem running
	 * single descriptors, but can be if running multiple concurrent
	 * descriptors. Reworking the driver to throttle to single requests
	 * is impractical, thus the workaround is to limit the AXI pipeline
	 * to a depth of 1 (from it's default of 4) to preclude this situation
	 * from occurring.
	 */
	wr_reg32(&ctrl->mcr, (rd_reg32(&ctrl->mcr) & ~(MCFGR_AXIPIPE_MASK)) |
			((1 << MCFGR_AXIPIPE_SHIFT) & MCFGR_AXIPIPE_MASK));

	/*
	 * Detect and enable JobRs
	 * First, find out how many ring spec'ed, allocate references
	 * for all, then go probe each one.
	 */
	rspec = 0;
	for_each_available_child_of_node(nprop, np)
		if (of_device_is_compatible(np, "fsl,sec-v4.0-job-ring") ||
		    of_device_is_compatible(np, "fsl,sec4.0-job-ring"))
			rspec++;

	ctrlpriv->jrpdev = xzalloc(sizeof(struct device_d *) * rspec);

	ring = 0;
	ctrlpriv->total_jobrs = 0;
	for_each_available_child_of_node(nprop, np) {
		if (of_device_is_compatible(np, "fsl,sec-v4.0-job-ring") ||
		    of_device_is_compatible(np, "fsl,sec4.0-job-ring")) {
			struct device_d *jrdev;

			jrdev = of_platform_device_create(np, dev);
			if (!jrdev)
				continue;

			ret = caam_jr_probe(jrdev);
			if (ret) {
				dev_err(dev, "Could not add jobring %d\n", ring);
				return ret;
			}

			ctrlpriv->jrpdev[ring] = jrdev;
			ctrlpriv->jr[ring] = (struct caam_job_ring __iomem __force *)
					     ((__force uint8_t *)ctrl +
					     (ring + JR_BLOCK_NUMBER) *
					      BLOCK_OFFSET);
			ctrlpriv->total_jobrs++;
			ring++;
		}
	}

	/* Check to see if QI present. If so, enable */
	ctrlpriv->qi_present = !!(comp_params & CTPR_MS_QI_MASK);
	if (ctrlpriv->qi_present) {
		ctrlpriv->qi = (struct caam_queue_if __iomem __force *)
			       ((__force uint8_t *)ctrl +
				 BLOCK_OFFSET * QI_BLOCK_NUMBER);
		/* This is all that's required to physically enable QI */
		wr_reg32(&ctrlpriv->qi->qi_control_lo, QICTL_DQEN);
	}

	/* If no QI and no rings specified, quit and go home */
	if ((!ctrlpriv->qi_present) && (!ctrlpriv->total_jobrs)) {
		dev_err(dev, "no queues configured, terminating\n");
		caam_remove(dev);
		return -ENOMEM;
	}

	cha_vid_ls = rd_reg32(&ctrl->perfmon.cha_id_ls);

	/* habv4_need_rng_software_self_test is determined by habv4_get_status() */
	if (IS_ENABLED(CONFIG_CRYPTO_DEV_FSL_CAAM_RNG_SELF_TEST) &&
	    habv4_need_rng_software_self_test) {
		u8 caam_era;
		u8 rngvid;
		u8 rngrev;

		caam_era = (rd_reg32(&ctrl->perfmon.ccb_id) & CCBVID_ERA_MASK) >> CCBVID_ERA_SHIFT;
		rngvid = (cha_vid_ls & CHAVID_LS_RNGVID_MASK) >> CHAVID_LS_RNGVID_SHIFT;
		rngrev = (rd_reg32(&ctrl->perfmon.cha_rev_ls) & CRNR_LS_RNGRN_MASK) >> CRNR_LS_RNGRN_SHIFT;

		ret = caam_rng_self_test(ctrlpriv->jrpdev[0], caam_era, rngvid, rngrev);
		if (ret != 0) {
			caam_remove(dev);
			return ret;
		}
	}

	/*
	 * If SEC has RNG version >= 4 and RNG state handle has not been
	 * already instantiated, do RNG instantiation
	 */
	if ((cha_vid_ls & CHA_ID_LS_RNG_MASK) >> CHA_ID_LS_RNG_SHIFT >= 4) {
		ctrlpriv->rng4_sh_init =
			rd_reg32(&ctrl->r4tst[0].rdsta);
		/*
		 * If the secure keys (TDKEK, JDKEK, TDSK), were already
		 * generated, signal this to the function that is instantiating
		 * the state handles. An error would occur if RNG4 attempts
		 * to regenerate these keys before the next POR.
		 */
		gen_sk = ctrlpriv->rng4_sh_init & RDSTA_SKVN ? 0 : 1;
		ctrlpriv->rng4_sh_init &= RDSTA_IFMASK;
		do {
			int inst_handles =
				rd_reg32(&ctrl->r4tst[0].rdsta) & RDSTA_IFMASK;
			/*
			 * If either SH were instantiated by somebody else
			 * (e.g. u-boot) then it is assumed that the entropy
			 * parameters are properly set and thus the function
			 * setting these (kick_trng(...)) is skipped.
			 * Also, if a handle was instantiated, do not change
			 * the TRNG parameters.
			 */
			if (!(ctrlpriv->rng4_sh_init || inst_handles)) {
				dev_dbg(dev, "Entropy delay = %u\n", ent_delay);
				kick_trng(dev, ent_delay);
				ent_delay += 400;
			}
			/*
			 * if instantiate_rng(...) fails, the loop will rerun
			 * and the kick_trng(...) function will modfiy the
			 * upper and lower limits of the entropy sampling
			 * interval, leading to a sucessful initialization of
			 * the RNG.
			 */
			ret = instantiate_rng(dev, inst_handles, gen_sk);
		} while ((ret == -EAGAIN) && (ent_delay < RTSDCTL_ENT_DLY_MAX));

		if (ret) {
			dev_err(dev, "failed to instantiate RNG");
			caam_remove(dev);
			return ret;
		}
		/*
		 * Set handles init'ed by this module as the complement of the
		 * already initialized ones
		 */
		ctrlpriv->rng4_sh_init = ~ctrlpriv->rng4_sh_init & RDSTA_IFMASK;

		/* Enable RDB bit so that RNG works faster */
		clrsetbits_32(&ctrl->scfgr, 0, SCFGR_RDBENABLE);
	}

	if (IS_ENABLED(CONFIG_CRYPTO_DEV_FSL_CAAM_RNG)) {
		ret = caam_rng_probe(dev, ctrlpriv->jrpdev[0]);
		if (ret) {
			dev_err(dev, "failed to instantiate RNG device");
			caam_remove(dev);
			return ret;
		}
	}

	/* NOTE: RTIC detection ought to go here, around Si time */
	caam_id = (u64)rd_reg32(&ctrl->perfmon.caam_id_ms) << 32 |
		  (u64)rd_reg32(&ctrl->perfmon.caam_id_ls);

	/* Report "alive" for developer to see */
	dev_dbg(dev, "device ID = 0x%016llx (Era %d)\n", caam_id,
		 caam_get_era(ctrl));
	dev_dbg(dev, "job rings = %d, qi = %d\n",
		 ctrlpriv->total_jobrs, ctrlpriv->qi_present);

	return 0;
}

static __maybe_unused struct of_device_id caam_match[] = {
	{
		.compatible = "fsl,sec-v4.0",
	}, {
		.compatible = "fsl,sec4.0",
	},
	{},
};

static struct driver_d caam_driver = {
	.name	= "caam",
	.probe	= caam_probe,
	.of_compatible = DRV_OF_COMPAT(caam_match),
};
device_platform_driver(caam_driver);
