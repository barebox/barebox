// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2019 NXP
 */

#define pr_fmt(fmt) "imx8m-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <soc/imx8m/ddr.h>
#include <mach/generic.h>
#include <mach/imx8m-regs.h>
#include <mach/imx8m-ccm-regs.h>

#define SRC_DDRC_RCR_ADDR MX8MQ_SRC_DDRC_RCR_ADDR

static void ddr_cfg_umctl2(struct dram_cfg_param *ddrc_cfg, int num)
{
	int i = 0;

	for (i = 0; i < num; i++) {
		reg32_write((unsigned long)ddrc_cfg->reg, ddrc_cfg->val);
		ddrc_cfg++;
	}
}

static int imx8m_ddr_init(unsigned long src_ddrc_rcr,
			  struct dram_timing_info *dram_timing)
{
	unsigned int tmp, initial_drate, target_freq;
	int ret;

	pr_debug("start DRAM init\n");

	pr_debug("cfg clk\n");

	/* disable iso */
	reg32_write(0x303A00EC, 0x0000ffff); /* PGC_CPU_MAPPING */
	reg32setbit(0x303A00F8, 5); /* PU_PGC_SW_PUP_REQ */

	initial_drate = dram_timing->fsp_msg[0].drate;
	/* default to the frequency point 0 clock */
	ddrphy_init_set_dfi_clk(initial_drate);

	/* D-aasert the presetn */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000006);

	/* Step2: Program the dwc_ddr_umctl2 registers */
	pr_debug("ddrc config start\n");
	ddr_cfg_umctl2(dram_timing->ddrc_cfg, dram_timing->ddrc_cfg_num);
	pr_debug("ddrc config done\n");

	/* Step3: De-assert reset signal(core_ddrc_rstn & aresetn_n) */
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000004);
	reg32_write(SRC_DDRC_RCR_ADDR, 0x8F000000);

	/*
	 * Step4: Disable auto-refreshes, self-refresh, powerdown, and
	 * assertion of dfi_dram_clk_disable by setting RFSHCTL3.dis_auto_refresh = 1,
	 * PWRCTL.powerdown_en = 0, and PWRCTL.selfref_en = 0,
	 * PWRCTL.en_dfi_dram_clk_disable = 0
	 */
	reg32_write(DDRC_DBG1(0), 0x00000000);
	reg32_write(DDRC_RFSHCTL3(0), 0x0000001);
	reg32_write(DDRC_PWRCTL(0), 0xa0);

	/* if ddr type is LPDDR4, do it */
	tmp = reg32_read(DDRC_MSTR(0));
	if (tmp & (0x1 << 5))
		reg32_write(DDRC_DDR_SS_GPR0, 0x01); /* LPDDR4 mode */

	/* determine the initial boot frequency */
	target_freq = reg32_read(DDRC_MSTR2(0)) & 0x3;
	target_freq = (tmp & (0x1 << 29)) ? target_freq : 0x0;

	/* Step5: Set SWCT.sw_done to 0 */
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* Set the default boot frequency point */
	clrsetbits_le32(DDRC_DFIMISC(0), (0x1f << 8), target_freq << 8);
	/* Step6: Set DFIMISC.dfi_init_complete_en to 0 */
	clrbits_le32(DDRC_DFIMISC(0), 0x1);

	/* Step7: Set SWCTL.sw_done to 1; need to polling SWSTAT.sw_done_ack */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	do {
		tmp = reg32_read(DDRC_SWSTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/*
	 * Step8 ~ Step13: Start PHY initialization and training by
	 * accessing relevant PUB registers
	 */
	pr_debug("ddrphy config start\n");

	ret = ddr_cfg_phy(dram_timing);
	if (ret)
		return ret;

	pr_debug("ddrphy config done\n");

	/*
	 * step14 CalBusy.0 =1, indicates the calibrator is actively
	 * calibrating. Wait Calibrating done.
	 */
	do {
		tmp = reg32_read(DDRPHY_CalBusy(0));
	} while ((tmp & 0x1));

	pr_debug("ddrphy calibration done\n");

	/* Step15: Set SWCTL.sw_done to 0 */
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* Step16: Set DFIMISC.dfi_init_start to 1 */
	setbits_le32(DDRC_DFIMISC(0), (0x1 << 5));

	/* Step17: Set SWCTL.sw_done to 1; need to polling SWSTAT.sw_done_ack */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	do {
		tmp = reg32_read(DDRC_SWSTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/* Step18: Polling DFISTAT.dfi_init_complete = 1 */
	do {
		tmp = reg32_read(DDRC_DFISTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/* Step19: Set SWCTL.sw_done to 0 */
	reg32_write(DDRC_SWCTL(0), 0x00000000);

	/* Step20: Set DFIMISC.dfi_init_start to 0 */
	clrbits_le32(DDRC_DFIMISC(0), (0x1 << 5));

	/* Step21: optional */

	/* Step22: Set DFIMISC.dfi_init_complete_en to 1 */
	setbits_le32(DDRC_DFIMISC(0), 0x1);

	/* Step23: Set PWRCTL.selfref_sw to 0 */
	clrbits_le32(DDRC_PWRCTL(0), (0x1 << 5));

	/* Step24: Set SWCTL.sw_done to 1; need polling SWSTAT.sw_done_ack */
	reg32_write(DDRC_SWCTL(0), 0x00000001);
	do {
		tmp = reg32_read(DDRC_SWSTAT(0));
	} while ((tmp & 0x1) == 0x0);

	/* Step25: Wait for dwc_ddr_umctl2 to move to normal operating mode by monitoring
	 * STAT.operating_mode signal */
	do {
		tmp = reg32_read(DDRC_STAT(0));
	} while ((tmp & 0x3) != 0x1);

	/* Step26: Set back register in Step4 to the original values if desired */
	reg32_write(DDRC_RFSHCTL3(0), 0x0000000);
	/* enable selfref_en by default */
	setbits_le32(DDRC_PWRCTL(0), 0x1 << 3);

	/* enable port 0 */
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	pr_debug(" ddrmix config done\n");

	return 0;
}

/*
 * We store the timing parameters here. the TF-A will pick these up.
 * Note that the timing used we leave the driver with is a PLL bypass 25MHz
 * mode. So if your board runs horribly slow you'll likely have to provide a
 * TF-A binary.
 */
#define IMX8M_SAVED_DRAM_TIMING_BASE		0x180000

int imx8mm_ddr_init(struct dram_timing_info *dram_timing)
{
	unsigned long src_ddrc_rcr = MX8M_SRC_DDRC_RCR_ADDR;
	int ret;

	/* Step1: Follow the power up procedure */
	reg32_write(src_ddrc_rcr, 0x8f00001f);
	reg32_write(src_ddrc_rcr, 0x8f00000f);

	ret = imx8m_ddr_init(src_ddrc_rcr, dram_timing);
	if (ret)
		return ret;

	/* save the dram timing config into memory */
	dram_config_save(dram_timing, IMX8M_SAVED_DRAM_TIMING_BASE);

	return 0;
}

int imx8mq_ddr_init(struct dram_timing_info *dram_timing)
{
	unsigned long src_ddrc_rcr = MX8MQ_SRC_DDRC_RCR_ADDR;
	int ret;

	/* Step1: Follow the power up procedure */
	reg32_write(src_ddrc_rcr + 0x04, 0x8f00000f);
	reg32_write(src_ddrc_rcr, 0x8f00000f);
	reg32_write(src_ddrc_rcr + 0x04, 0x8f000000);

	ret = imx8m_ddr_init(src_ddrc_rcr, dram_timing);
	if (ret)
		return ret;

	/* save the dram timing config into memory */
	dram_config_save(dram_timing, IMX8M_SAVED_DRAM_TIMING_BASE);

	return 0;
}
