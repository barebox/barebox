// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2024 Intel Corporation <www.intel.com>
 *
 */
#define pr_fmt(fmt) "agilex5-sdram: " fmt

#include <common.h>
#include <errno.h>
#include "iossm_mailbox.h"
#include <xfuncs.h>
#include <linux/sizes.h>
#include <linux/bitfield.h>
#include <mach/socfpga/soc64-firewall.h>
#include <mach/socfpga/soc64-handoff.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-sdram.h>
#include <mach/socfpga/soc64-system-manager.h>

/* MPFE NOC registers */
#define F2SDRAM_SIDEBAND_FLAGOUTSET0	0x50
#define F2SDRAM_SIDEBAND_FLAGOUTSTATUS0	0x58
#define SIDEBANDMGR_FLAGOUTSET0_REG	SOCFPGA_F2SDRAM_MGR_ADDRESS +\
					F2SDRAM_SIDEBAND_FLAGOUTSET0
#define SIDEBANDMGR_FLAGOUTSTATUS0_REG	SOCFPGA_F2SDRAM_MGR_ADDRESS +\
					F2SDRAM_SIDEBAND_FLAGOUTSTATUS0

#define PORT_EMIF_CONFIG_OFFSET 4

/* Reset type */
enum reset_type {
	POR_RESET,
	WARM_RESET,
	COLD_RESET,
	NCONFIG,
	JTAG_CONFIG,
	RSU_RECONFIG
};

static phys_addr_t io96b_csr_reg_addr[] = {
	0x18400000, /* IO96B_0 CSR registers address */
	0x18800000  /* IO96B_1 CSR registers address */
};

static enum reset_type get_reset_type(u32 val)
{
	return (val & ALT_SYSMGR_SCRATCH_REG_3_DDR_RESET_TYPE_MASK) >>
		ALT_SYSMGR_SCRATCH_REG_3_DDR_RESET_TYPE_SHIFT;
}

static int set_mpfe_config(void)
{
	/* Set mpfe_lite_intfcsel */
	setbits_le32(IOMEM(SOCFPGA_SYSMGR_ADDRESS) + SYSMGR_SOC64_MPFE_CONFIG, BIT(2));

	/* Set mpfe_lite_active */
	setbits_le32(IOMEM(SOCFPGA_SYSMGR_ADDRESS) + SYSMGR_SOC64_MPFE_CONFIG, BIT(8));

	pr_debug("%s: mpfe_config: 0x%x\n", __func__,
	      readl(IOMEM(SOCFPGA_SYSMGR_ADDRESS) + SYSMGR_SOC64_MPFE_CONFIG));

	return 0;
}

static bool is_ddr_init_hang(void)
{
	u32 reg = readl(IOMEM(SOCFPGA_SYSMGR_ADDRESS) +
			SYSMGR_SOC64_BOOT_SCRATCH_POR0);

	if (reg & ALT_SYSMGR_SCRATCH_REG_POR_0_DDR_PROGRESS_MASK)
		return true;

	return false;
}

static void ddr_init_inprogress(bool start)
{
	if (start)
		setbits_le32(IOMEM(SOCFPGA_SYSMGR_ADDRESS) +
				SYSMGR_SOC64_BOOT_SCRATCH_POR0,
				ALT_SYSMGR_SCRATCH_REG_POR_0_DDR_PROGRESS_MASK);
	else
		clrbits_le32(IOMEM(SOCFPGA_SYSMGR_ADDRESS) +
				SYSMGR_SOC64_BOOT_SCRATCH_POR0,
				ALT_SYSMGR_SCRATCH_REG_POR_0_DDR_PROGRESS_MASK);
}

static int populate_ddr_handoff(struct altera_sdram_plat *plat, struct io96b_info *io96b_ctrl)
{
	int i;
	u32 len = SOC64_HANDOFF_SDRAM_LEN;
	u32 handoff_table[len];

	/* Read handoff for DDR configuration */
	socfpga_handoff_read((void *)SOC64_HANDOFF_SDRAM, handoff_table, len);

	/* Read handoff - dual port
	   FIXME: Intel u-boot has a patch that HACKs this to 0
	   https://github.com/altera-opensource/meta-intel-fpga-refdes/ \
	   blob/master/recipes-bsp/u-boot/files/v1-0001-HSD-15015933655-ddr-altera-agilex5-Hack-dual-port-DO-NOT-MERGE.patch
	   Patch doesn't say why or what is broken here: handoff files? dualport RAM access?
	 */
	//plat->dualport = FIELD_GET(BIT(0), handoff_table[PORT_EMIF_CONFIG_OFFSET]);
	plat->dualport = 0;
	pr_debug("%s: dualport from handoff: 0x%x\n", __func__, plat->dualport);

	if (plat->dualport)
		io96b_ctrl->num_port = 2;
	else
		io96b_ctrl->num_port = 1;

	/* Read handoff - dual EMIF */
	plat->dualemif = FIELD_GET(BIT(1), handoff_table[PORT_EMIF_CONFIG_OFFSET]);
	pr_debug("%s: dualemif from handoff: 0x%x\n", __func__, plat->dualemif);

	if (plat->dualemif)
		io96b_ctrl->num_instance = 2;
	else
		io96b_ctrl->num_instance = 1;

	/* Assign IO96B CSR base address if it is valid */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		io96b_ctrl->io96b[i].io96b_csr_addr = io96b_csr_reg_addr[i];
		pr_debug("%s: IO96B 0x%llx CSR enabled\n", __func__
			, io96b_ctrl->io96b[i].io96b_csr_addr);
	}

	return 0;
}

static int config_mpfe_sideband_mgr(struct altera_sdram_plat *plat)
{
	/* Dual port setting */
	if (plat->dualport)
		setbits_le32(SIDEBANDMGR_FLAGOUTSET0_REG, BIT(4));

	/* Dual EMIF setting */
	if (plat->dualemif) {
		set_mpfe_config();
		setbits_le32(SIDEBANDMGR_FLAGOUTSET0_REG, BIT(5));
	}

	pr_debug("%s: SIDEBANDMGR_FLAGOUTSTATUS0: 0x%x\n", __func__,
	      readl(SIDEBANDMGR_FLAGOUTSTATUS0_REG));

	return 0;
}

static void config_ccu_mgr(struct altera_sdram_plat *plat)
{
	if (plat->dualport || plat->dualemif) {
		pr_debug("%s: config interleaving on ccu reg\n", __func__);
		agilex5_security_interleaving_on();
	} else {
		pr_debug("%s: config interleaving off ccu reg\n", __func__);
		agilex5_security_interleaving_off();
	}
}

static bool hps_ocram_dbe_status(void)
{
	u32 reg = readl(IOMEM(SOCFPGA_SYSMGR_ADDRESS) +
			SYSMGR_SOC64_BOOT_SCRATCH_COLD3);

	if (reg & ALT_SYSMGR_SCRATCH_REG_3_OCRAM_DBE_MASK)
		return true;

	return false;
}

static bool ddr_ecc_dbe_status(void)
{
	u32 reg = readl(IOMEM(SOCFPGA_SYSMGR_ADDRESS) +
			SYSMGR_SOC64_BOOT_SCRATCH_COLD3);

	if (reg & ALT_SYSMGR_SCRATCH_REG_3_DDR_DBE_MASK)
		return true;

	return false;
}

static void sdram_set_firewall(phys_size_t hw_size)
{
	phys_size_t value;
	u32 lower, upper;

	value = SOCFPGA_AGILEX5_DDR_BASE;
	/* Keep first 1MB of SDRAM memory region as secure region when
	 * using ATF flow, where the ATF code is located.
	 */
	value += SZ_1M;

	/* Setting non-secure MPU region base and base extended */
	lower = lower_32_bits(value);
	upper = upper_32_bits(value);
	FW_MPU_DDR_SCR_WRITEL(lower, FW_MPU_DDR_SCR_MPUREGION0ADDR_BASE);
	FW_MPU_DDR_SCR_WRITEL(upper & 0xff, FW_MPU_DDR_SCR_MPUREGION0ADDR_BASEEXT);
	FW_F2SDRAM_DDR_SCR_WRITEL(lower, FW_F2SDRAM_DDR_SCR_REGION0ADDR_BASE);
	FW_F2SDRAM_DDR_SCR_WRITEL(upper & 0xff, FW_F2SDRAM_DDR_SCR_REGION0ADDR_BASEEXT);

	/* Setting non-secure Non-MPU region base and base extended */
	FW_MPU_DDR_SCR_WRITEL(lower, FW_MPU_DDR_SCR_NONMPUREGION0ADDR_BASE);
	FW_MPU_DDR_SCR_WRITEL(upper & 0xff, FW_MPU_DDR_SCR_NONMPUREGION0ADDR_BASEEXT);

	/* Setting non-secure MPU limit and limit extended */
	value = SOCFPGA_AGILEX5_DDR_BASE + hw_size - 1;

	lower = lower_32_bits(value);
	upper = upper_32_bits(value);

	FW_MPU_DDR_SCR_WRITEL(lower, FW_MPU_DDR_SCR_MPUREGION0ADDR_LIMIT);
	FW_MPU_DDR_SCR_WRITEL(upper & 0xff, FW_MPU_DDR_SCR_MPUREGION0ADDR_LIMITEXT);

	/* Setting non-secure Non-MPU limit and limit extended */
	FW_MPU_DDR_SCR_WRITEL(lower, FW_MPU_DDR_SCR_NONMPUREGION0ADDR_LIMIT);
	FW_MPU_DDR_SCR_WRITEL(upper & 0xff, FW_MPU_DDR_SCR_NONMPUREGION0ADDR_LIMITEXT);
	FW_MPU_DDR_SCR_WRITEL(BIT(0) | BIT(8), FW_MPU_DDR_SCR_EN_SET);

	FW_F2SDRAM_DDR_SCR_WRITEL(lower, FW_F2SDRAM_DDR_SCR_REGION0ADDR_LIMIT);
	FW_F2SDRAM_DDR_SCR_WRITEL(upper & 0xff, FW_F2SDRAM_DDR_SCR_REGION0ADDR_LIMITEXT);
	FW_F2SDRAM_DDR_SCR_WRITEL(BIT(0), FW_F2SDRAM_DDR_SCR_EN_SET);
}

int agilex5_ddr_init_full(void)
{
	int ret;
	int i;
	phys_size_t hw_size;
	struct altera_sdram_plat plat;
	struct io96b_info io96b_ctrl;

	enum reset_type reset_t = get_reset_type(SOCFPGA_SYSMGR_ADDRESS +
						 SYSMGR_SOC64_BOOT_SCRATCH_COLD3);
	bool full_mem_init = false;

	/* DDR initialization progress status tracking */
	bool is_ddr_hang_be4_rst = is_ddr_init_hang();

	pr_debug("DDR: SDRAM init in progress ...\n");
	ddr_init_inprogress(true);

	plat.mpfe_base_addr = IOMEM(SOCFPGA_MPFE_CSR_ADDRESS);

	pr_debug("DDR: Address MPFE 0x%p\n", plat.mpfe_base_addr);

	/* Populating DDR handoff data */
	pr_debug("DDR: Checking SDRAM configuration in progress ...\n");
	ret = populate_ddr_handoff(&plat, &io96b_ctrl);
	if (ret) {
		pr_debug("DDR: Failed to populate DDR handoff\n");
		return ret;
	}

	/* Configuring MPFE sideband manager registers - dual port & dual emif*/
	ret = config_mpfe_sideband_mgr(&plat);
	if (ret) {
		pr_debug("DDR: Failed to configure dual port dual emif\n");
		return ret;
	}

	/* Configuring Interleave/Non-interleave ccu registers */
	config_ccu_mgr(&plat);

	/* Configure if polling is needed for IO96B GEN PLL locked */
	io96b_ctrl.ckgen_lock = true;

	/* Ensure calibration status passing */
	io96b_init_mem_cal(&io96b_ctrl);

	/* Initiate IOSSM mailbox */
	io96b_mb_init(&io96b_ctrl);

	/* Need to trigger re-calibration for DDR DBE */
	if (ddr_ecc_dbe_status()) {
		for (i = 0; i < io96b_ctrl.num_instance; i++)
			io96b_ctrl.io96b[i].cal_status = false;

		io96b_ctrl.overall_cal_status = false;
	}

	/* Trigger re-calibration if calibration failed */
	if (!(io96b_ctrl.overall_cal_status)) {
		pr_debug("DDR: Re-calibration in progress...\n");
		io96b_trig_mem_cal(&io96b_ctrl);
	}

	pr_debug("DDR: Calibration success\n");

	/* DDR type, DDR size and ECC status) */
	ret = io96b_get_mem_technology(&io96b_ctrl);
	if (ret) {
		pr_debug("DDR: Failed to get DDR type\n");
		return ret;
	}

	ret = io96b_get_mem_width_info(&io96b_ctrl);
	if (ret) {
		pr_debug("DDR: Failed to get DDR size\n");
		return ret;
	}

	hw_size = (phys_size_t)io96b_ctrl.overall_size * SZ_1G / SZ_8;

	pr_debug("%s: %lld MiB\n", io96b_ctrl.ddr_type, hw_size >> 20);

	ret = io96b_ecc_enable_status(&io96b_ctrl);
	if (ret) {
		pr_debug("DDR: Failed to get DDR ECC status\n");
		return ret;
	}

	/* Is HPS cold or warm reset? If yes, Skip full memory initialization if ECC
	 *  enabled to preserve memory content
	 */
	if (io96b_ctrl.ecc_status) {
		full_mem_init = hps_ocram_dbe_status() | ddr_ecc_dbe_status() |
				is_ddr_hang_be4_rst;
		if (full_mem_init || !(reset_t == WARM_RESET || reset_t == COLD_RESET)) {
			ret = io96b_bist_mem_init_start(&io96b_ctrl);
			if (ret) {
				pr_debug("DDR: Failed to fully initialize DDR memory\n");
				return ret;
			}
		}

		pr_debug("SDRAM-ECC: Initialized success\n");
	}

	sdram_set_firewall(hw_size);

	/* Firewall setting for MPFE CSR */
	/* IO96B0_reg */
	writel(0x1, SOCFPGA_MPFE_CSR_ADDRESS + 0xd00);
	/* IO96B1_reg */
	writel(0x1, SOCFPGA_MPFE_CSR_ADDRESS + 0xd04);
	/* noc_csr */
	writel(0x1, SOCFPGA_MPFE_CSR_ADDRESS + 0xd08);

	pr_debug("DDR: firewall init success\n");

	/* Ending DDR driver initialization success tracking */
	ddr_init_inprogress(false);

	pr_debug("DDR: init success\n");

	return 0;
}
