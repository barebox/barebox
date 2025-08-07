// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2023 Intel Corporation <www.intel.com>
 */

#include <common.h>
#include <io.h>
#include <asm/system.h>
#include <dt-bindings/clock/intel,agilex5-clkmgr.h>
#include <linux/bitops.h>
#include <clock.h>
#include <mach/socfpga/agilex5-clk.h>
#include <mach/socfpga/generic.h>
#include <mach/socfpga/soc64-handoff.h>
#include <mach/socfpga/soc64-regs.h>

#define MPIDR_AFF1_OFFSET	8
#define MPIDR_AFF1_MASK		0x3
#define CORE0		1
#define CORE1		2
#define CORE2		3
#define CORE3		4

/* Derived from l4_main_clk (PSS clock) */
#define COUNTER_FREQUENCY_REAL	400000000

struct socfpga_clk_plat {
	void __iomem *regs;
};

static void cm_wait_for_lock(u32 mask)
{
	register uint32_t inter_val;

	do {
		inter_val = readl(SOCFPGA_CLKMGR_ADDRESS + CLKMGR_STAT) & mask;
	} while (inter_val != mask);
}

/* function to poll in the fsm busy bit */
static void cm_wait_for_fsm(void)
{
	register u32 inter_val;

	do {
		inter_val = readl(SOCFPGA_CLKMGR_ADDRESS + CLKMGR_STAT) &
				CLKMGR_STAT_BUSY;
	} while (inter_val);
}

/*
 * function to write the bypass register which requires a poll of the
 * busy bit
 */
static void clk_write_bypass_mainpll(struct socfpga_clk_plat *plat, u32 val)
{
	CM_REG_WRITEL(plat, val, CLKMGR_MAINPLL_BYPASS);
	cm_wait_for_fsm();
}

static void clk_write_bypass_perpll(struct socfpga_clk_plat *plat, u32 val)
{
	CM_REG_WRITEL(plat, val, CLKMGR_PERPLL_BYPASS);
	cm_wait_for_fsm();
}

/* function to write the ctrl register which requires a poll of the busy bit */
static void clk_write_ctrl(struct socfpga_clk_plat *plat, u32 val)
{
	CM_REG_WRITEL(plat, val, CLKMGR_CTRL);
	cm_wait_for_fsm();
}

#define MEMBUS_MAINPLL				0
#define MEMBUS_PERPLL				1
#define MEMBUS_TIMEOUT				1000

#define MEMBUS_CLKSLICE_REG				0x27
#define MEMBUS_SYNTHCALFOSC_INIT_CENTERFREQ_REG		0xb3
#define MEMBUS_SYNTHPPM_WATCHDOGTMR_VF01_REG		0xe6
#define MEMBUS_CALCLKSLICE0_DUTY_LOCOVR_REG		0x03
#define MEMBUS_CALCLKSLICE1_DUTY_LOCOVR_REG		0x07

static const struct {
	u32 reg;
	u32 val;
	u32 mask;
} membus_pll[] = {
	{
		MEMBUS_CLKSLICE_REG,
		/*
		 * BIT[7:7]
		 * Enable source synchronous mode
		 */
		BIT(7),
		BIT(7)
	},
	{
		MEMBUS_SYNTHCALFOSC_INIT_CENTERFREQ_REG,
		/*
		 * BIT[0:0]
		 * Sets synthcalfosc_init_centerfreq=1 to limit overshoot
		 * frequency during lock
		 */
		BIT(0),
		BIT(0)
	},
	{
		MEMBUS_SYNTHPPM_WATCHDOGTMR_VF01_REG,
		/*
		 * BIT[0:0]
		 * Sets synthppm_watchdogtmr_vf0=1 to give the pll more time
		 * to settle before lock is asserted.
		 */
		BIT(0),
		BIT(0)
	},
	{
		MEMBUS_CALCLKSLICE0_DUTY_LOCOVR_REG,
		/*
		 * BIT[6:0]
		 * Centering duty cycle for clkslice0 output
		 */
		0x4a,
		GENMASK(6, 0)
	},
	{
		MEMBUS_CALCLKSLICE1_DUTY_LOCOVR_REG,
		/*
		 * BIT[6:0]
		 * Centering duty cycle for clkslice1 output
		 */
		0x4a,
		GENMASK(6, 0)
	},
};

static int membus_wait_for_req(struct socfpga_clk_plat *plat, u32 pll,
			       int timeout)
{
	int cnt = 0;
	u32 req_status;

	if (pll == MEMBUS_MAINPLL)
		req_status = CM_REG_READL(plat, CLKMGR_MAINPLL_MEM);
	else
		req_status = CM_REG_READL(plat, CLKMGR_PERPLL_MEM);

	while ((cnt < timeout) && (req_status & CLKMGR_MEM_REQ_SET_MSK)) {
		if (pll == MEMBUS_MAINPLL)
			req_status = CM_REG_READL(plat, CLKMGR_MAINPLL_MEM);
		else
			req_status = CM_REG_READL(plat, CLKMGR_PERPLL_MEM);
		cnt++;
	}

	if (cnt >= timeout)
		return -ETIMEDOUT;

	return 0;
}

static int membus_write_pll(struct socfpga_clk_plat *plat, u32 pll,
			    u32 addr_offset, u32 wdat, int timeout)
{
	u32 addr;
	u32 val;

	addr = ((addr_offset | CLKMGR_MEM_ADDR_START) & CLKMGR_MEM_ADDR_MASK);

	val = (CLKMGR_MEM_REQ_SET_MSK | CLKMGR_MEM_WR_SET_MSK |
	       (wdat << CLKMGR_MEM_WDAT_LSB_OFFSET) | addr);

	if (pll == MEMBUS_MAINPLL)
		CM_REG_WRITEL(plat, val, CLKMGR_MAINPLL_MEM);
	else
		CM_REG_WRITEL(plat, val, CLKMGR_PERPLL_MEM);

	return membus_wait_for_req(plat, pll, timeout);
}

static int membus_read_pll(struct socfpga_clk_plat *plat, u32 pll,
			   u32 addr_offset, u32 *rdata, int timeout)
{
	u32 addr;
	u32 val;

	addr = ((addr_offset | CLKMGR_MEM_ADDR_START) & CLKMGR_MEM_ADDR_MASK);

	val = ((CLKMGR_MEM_REQ_SET_MSK & ~CLKMGR_MEM_WR_SET_MSK) | addr);

	if (pll == MEMBUS_MAINPLL)
		CM_REG_WRITEL(plat, val, CLKMGR_MAINPLL_MEM);
	else
		CM_REG_WRITEL(plat, val, CLKMGR_PERPLL_MEM);

	*rdata = 0;

	if (membus_wait_for_req(plat, pll, timeout))
		return -ETIMEDOUT;

	if (pll == MEMBUS_MAINPLL)
		*rdata = CM_REG_READL(plat, CLKMGR_MAINPLL_MEMSTAT);
	else
		*rdata = CM_REG_READL(plat, CLKMGR_PERPLL_MEMSTAT);

	return 0;
}

static void membus_pll_configs(struct socfpga_clk_plat *plat, u32 pll)
{
	int i;
	u32 rdata;

	for (i = 0; i < ARRAY_SIZE(membus_pll); i++) {
		membus_read_pll(plat, pll, membus_pll[i].reg,
				&rdata, MEMBUS_TIMEOUT);
		membus_write_pll(plat, pll, membus_pll[i].reg,
			 ((rdata & ~membus_pll[i].mask) | membus_pll[i].val),
			 MEMBUS_TIMEOUT);
	}
}

static u32 calc_vocalib_pll(u32 pllm, u32 pllglob)
{
	u32 mdiv, refclkdiv, arefclkdiv, drefclkdiv, mscnt, hscnt, vcocalib;

	mdiv = pllm & CLKMGR_PLLM_MDIV_MASK;
	arefclkdiv = (pllglob & CLKMGR_PLLGLOB_AREFCLKDIV_MASK) >>
		      CLKMGR_PLLGLOB_AREFCLKDIV_OFFSET;
	drefclkdiv = (pllglob & CLKMGR_PLLGLOB_DREFCLKDIV_MASK) >>
		      CLKMGR_PLLGLOB_DREFCLKDIV_OFFSET;
	refclkdiv = (pllglob & CLKMGR_PLLGLOB_REFCLKDIV_MASK) >>
		     CLKMGR_PLLGLOB_REFCLKDIV_OFFSET;
	mscnt = CLKMGR_VCOCALIB_MSCNT_CONST / (mdiv * BIT(drefclkdiv));
	if (!mscnt)
		mscnt = 1;
	hscnt = (mdiv * mscnt * BIT(drefclkdiv) / refclkdiv) -
		CLKMGR_VCOCALIB_HSCNT_CONST;
	vcocalib = (hscnt & CLKMGR_VCOCALIB_HSCNT_MASK) |
		   ((mscnt << CLKMGR_VCOCALIB_MSCNT_OFFSET) &
		     CLKMGR_VCOCALIB_MSCNT_MASK);

	return vcocalib;
}

/*
 * Setup clocks while making no assumptions about previous state of the clocks.
 */
static void clk_basic_init(struct socfpga_clk_plat *plat,
			   const struct cm_config * const cfg)
{
	u32 vcocalib;
	u32 cntfrq = COUNTER_FREQUENCY_REAL;
	u32 counter_freq = 0;


	if (!cfg)
		return;

	/* Always force clock manager into boot mode before any configuration */
	clk_write_ctrl(plat,
		       CM_REG_READL(plat, CLKMGR_CTRL) | CLKMGR_CTRL_BOOTMODE);

	/* Put both PLLs in bypass */
	clk_write_bypass_mainpll(plat, CLKMGR_BYPASS_MAINPLL_ALL);
	clk_write_bypass_perpll(plat, CLKMGR_BYPASS_PERPLL_ALL);

	/* Put both PLLs in Reset and Power Down */
	CM_REG_CLRBITS(plat, CLKMGR_MAINPLL_PLLGLOB,
		       CLKMGR_PLLGLOB_PD_MASK | CLKMGR_PLLGLOB_RST_MASK);
	CM_REG_CLRBITS(plat, CLKMGR_PERPLL_PLLGLOB,
		       CLKMGR_PLLGLOB_PD_MASK | CLKMGR_PLLGLOB_RST_MASK);

	/* setup main PLL dividers where calculate the vcocalib value */
	vcocalib = calc_vocalib_pll(cfg->main_pll_pllm, cfg->main_pll_pllglob);
	CM_REG_WRITEL(plat, cfg->main_pll_pllglob & ~CLKMGR_PLLGLOB_RST_MASK,
		      CLKMGR_MAINPLL_PLLGLOB);
	CM_REG_WRITEL(plat, cfg->main_pll_fdbck, CLKMGR_MAINPLL_FDBCK);
	CM_REG_WRITEL(plat, vcocalib, CLKMGR_MAINPLL_VCOCALIB);
	CM_REG_WRITEL(plat, cfg->main_pll_pllc0, CLKMGR_MAINPLL_PLLC0);
	CM_REG_WRITEL(plat, cfg->main_pll_pllc1, CLKMGR_MAINPLL_PLLC1);
	CM_REG_WRITEL(plat, cfg->main_pll_pllc2, CLKMGR_MAINPLL_PLLC2);
	CM_REG_WRITEL(plat, cfg->main_pll_pllc3, CLKMGR_MAINPLL_PLLC3);
	CM_REG_WRITEL(plat, cfg->main_pll_pllm, CLKMGR_MAINPLL_PLLM);
	CM_REG_WRITEL(plat, cfg->main_pll_nocclk, CLKMGR_MAINPLL_NOCCLK);
	CM_REG_WRITEL(plat, cfg->main_pll_nocdiv, CLKMGR_MAINPLL_NOCDIV);

	/* setup peripheral PLL dividers where calculate the vcocalib value */
	vcocalib = calc_vocalib_pll(cfg->per_pll_pllm, cfg->per_pll_pllglob);
	CM_REG_WRITEL(plat, cfg->per_pll_pllglob & ~CLKMGR_PLLGLOB_RST_MASK,
		      CLKMGR_PERPLL_PLLGLOB);
	CM_REG_WRITEL(plat, cfg->per_pll_fdbck, CLKMGR_PERPLL_FDBCK);
	CM_REG_WRITEL(plat, vcocalib, CLKMGR_PERPLL_VCOCALIB);
	CM_REG_WRITEL(plat, cfg->per_pll_pllc0, CLKMGR_PERPLL_PLLC0);
	CM_REG_WRITEL(plat, cfg->per_pll_pllc1, CLKMGR_PERPLL_PLLC1);
	CM_REG_WRITEL(plat, cfg->per_pll_pllc2, CLKMGR_PERPLL_PLLC2);
	CM_REG_WRITEL(plat, cfg->per_pll_pllc3, CLKMGR_PERPLL_PLLC3);
	CM_REG_WRITEL(plat, cfg->per_pll_pllm, CLKMGR_PERPLL_PLLM);
	CM_REG_WRITEL(plat, cfg->per_pll_emacctl, CLKMGR_PERPLL_EMACCTL);
	CM_REG_WRITEL(plat, cfg->per_pll_gpiodiv, CLKMGR_PERPLL_GPIODIV);

	/* Configure ping pong counters in control group */
	CM_REG_WRITEL(plat, cfg->ctl_emacactr, CLKMGR_CTL_EMACACTR);
	CM_REG_WRITEL(plat, cfg->ctl_emacbctr, CLKMGR_CTL_EMACBCTR);
	CM_REG_WRITEL(plat, cfg->ctl_emacptpctr, CLKMGR_CTL_EMACPTPCTR);
	CM_REG_WRITEL(plat, cfg->ctl_gpiodbctr, CLKMGR_CTL_GPIODBCTR);
	CM_REG_WRITEL(plat, cfg->ctl_s2fuser0ctr, CLKMGR_CTL_S2FUSER0CTR);
	CM_REG_WRITEL(plat, cfg->ctl_s2fuser1ctr, CLKMGR_CTL_S2FUSER1CTR);
	CM_REG_WRITEL(plat, cfg->ctl_psirefctr, CLKMGR_CTL_PSIREFCTR);
	CM_REG_WRITEL(plat, cfg->ctl_usb31ctr, CLKMGR_CTL_USB31CTR);
	CM_REG_WRITEL(plat, cfg->ctl_dsuctr, CLKMGR_CTL_DSUCTR);
	CM_REG_WRITEL(plat, cfg->ctl_core01ctr, CLKMGR_CTL_CORE01CTR);
	CM_REG_WRITEL(plat, cfg->ctl_core23ctr, CLKMGR_CTL_CORE23CTR);
	CM_REG_WRITEL(plat, cfg->ctl_core2ctr, CLKMGR_CTL_CORE2CTR);
	CM_REG_WRITEL(plat, cfg->ctl_core3ctr, CLKMGR_CTL_CORE3CTR);

	/* Take both PLL out of reset and power up */
	CM_REG_SETBITS(plat, CLKMGR_MAINPLL_PLLGLOB,
		       CLKMGR_PLLGLOB_PD_MASK | CLKMGR_PLLGLOB_RST_MASK);
	CM_REG_SETBITS(plat, CLKMGR_PERPLL_PLLGLOB,
		       CLKMGR_PLLGLOB_PD_MASK | CLKMGR_PLLGLOB_RST_MASK);

	/* Membus programming for mainpll */
	membus_pll_configs(plat, MEMBUS_MAINPLL);
	/* Membus programming for peripll */
	membus_pll_configs(plat, MEMBUS_PERPLL);

	/* Enable Main pll clkslices */
	CM_REG_SETBITS(plat, CLKMGR_MAINPLL_PLLC0, CLKMGR_PLLCX_EN_SET_MSK);
	CM_REG_SETBITS(plat, CLKMGR_MAINPLL_PLLC1, CLKMGR_PLLCX_EN_SET_MSK);
	CM_REG_SETBITS(plat, CLKMGR_MAINPLL_PLLC2, CLKMGR_PLLCX_EN_SET_MSK);
	CM_REG_SETBITS(plat, CLKMGR_MAINPLL_PLLC3, CLKMGR_PLLCX_EN_SET_MSK);

	/* Enable Periph pll clkslices */
	CM_REG_SETBITS(plat, CLKMGR_PERPLL_PLLC0, CLKMGR_PLLCX_EN_SET_MSK);
	CM_REG_SETBITS(plat, CLKMGR_PERPLL_PLLC1, CLKMGR_PLLCX_EN_SET_MSK);
	CM_REG_SETBITS(plat, CLKMGR_PERPLL_PLLC2, CLKMGR_PLLCX_EN_SET_MSK);
	CM_REG_SETBITS(plat, CLKMGR_PERPLL_PLLC3, CLKMGR_PLLCX_EN_SET_MSK);

	cm_wait_for_lock(CLKMGR_STAT_ALLPLL_LOCKED_MASK);

	CM_REG_WRITEL(plat, CLKMGR_LOSTLOCK_SET_MASK, CLKMGR_MAINPLL_LOSTLOCK);
	CM_REG_WRITEL(plat, CLKMGR_LOSTLOCK_SET_MASK, CLKMGR_PERPLL_LOSTLOCK);

	CM_REG_SETBITS(plat, CLKMGR_MAINPLL_PLLGLOB, CLKMGR_PLLGLOB_CLR_LOSTLOCK_BYPASS_MASK);
	CM_REG_SETBITS(plat, CLKMGR_PERPLL_PLLGLOB, CLKMGR_PLLGLOB_CLR_LOSTLOCK_BYPASS_MASK);

	/* Take all PLLs out of bypass */
	clk_write_bypass_mainpll(plat, 0);
	clk_write_bypass_perpll(plat, 0);

	/* Clear the loss of lock bits (write 1 to clear) */
	CM_REG_CLRBITS(plat, CLKMGR_INTRCLR,
		       CLKMGR_INTER_PERPLLLOST_MASK |
		       CLKMGR_INTER_MAINPLLLOST_MASK);

	/* Take all ping pong counters out of reset */
	CM_REG_CLRBITS(plat, CLKMGR_CTL_EXTCNTRST,
		       CLKMGR_CTL_EXTCNTRST_ALLCNTRST);

	/* Update with accurate clock frequency */
	if (current_el() == 3) {
		asm volatile("msr cntfrq_el0, %0" : : "r" (cntfrq) : "memory");
		asm volatile("mrs %0, cntfrq_el0" : "=r" (counter_freq));
	}

	/* Out of boot mode */
	clk_write_ctrl(plat, CM_REG_READL(plat, CLKMGR_CTRL) &
		       ~CLKMGR_CTRL_BOOTMODE);
}

int agilex5_clk_init(void)
{
	struct socfpga_clk_plat plat;
	const struct cm_config *cm_default_cfg = cm_get_default_config();

	plat.regs = (void __iomem *)SOCFPGA_CLKMGR_ADDRESS;

	clk_basic_init(&plat, cm_default_cfg);

	return 0;
}
