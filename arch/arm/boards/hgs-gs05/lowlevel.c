// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix
// SPDX-FileCopyrightText: Leica Geosystems AG

#include <debug_ll.h>

#include <asm/cache.h>
#include <asm/barebox-arm.h>
#include <asm/mmu.h>
#include <asm/system.h>

#include <boards/hgs/common.h>

#include <mach/imx/debug_ll.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx-gpio.h>
#include <mach/imx/imx8mm-regs.h>
#include <mach/imx/iomux-mx8mm.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/xload.h>
#include <soc/imx8m/ddr.h>

#include <mfd/pca9450.h>

#include <pbl/i2c.h>
#include <pbl/pmic.h>

#include <soc/imx8m/ddr.h>

static struct pmic_config pca9450_cfg[] = {
	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	{ PCA9450_BUCK123_DVS, 0x29 },

	/* Buck 1 DVS control through PMIC_STBY_REQ */
	{ PCA9450_BUCK1CTRL, 0x59 },

	/* Set DVS1 to 0.8v for suspend */
	{ PCA9450_BUCK1OUT_DVS1, 0x10 },

	/* increase VDD_DRAM to 0.95v for 3Ghz DDR */
	{ PCA9450_BUCK3OUT_DVS0, 0x1c },

	/*
	 * VDD_DRAM needs off in suspend, set B3_ENMODE=10
	 * (ON by PMIC_ON_REQ = H && PMIC_STBY_REQ = L)
	 */
	{ PCA9450_BUCK3CTRL, 0x4a },

	/* set VDD_SNVS_0V8 from default 0.85V */
	{ PCA9450_LDO2CTRL, 0xc0 },

	/* set WDOG_B_CFG to cold reset only LDO1/2 left enabled */
	{ PCA9450_RESET_CTRL, 0xa1 },
};

static void power_init_board(void)
{
	struct pbl_i2c *i2c;

	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SCL_I2C1_SCL);
	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SDA_I2C1_SDA);

	imx8mm_early_clock_init();
	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MM_I2C1_BASE_ADDR));

	pmic_configure(i2c, 0x25, pca9450_cfg, ARRAY_SIZE(pca9450_cfg));
}

extern struct dram_timing_info hgs_gs05_dram_timing;
extern char __dtb_z_imx8mm_hgs_gs05_start[];

static void start_atf(void)
{
	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	power_init_board();
	imx8mm_ddr_init(&hgs_gs05_dram_timing, DRAM_TYPE_LPDDR4);

	/* OP-TEE binary will be loaded at the 1G (start of RAM) */
	__imx8mm_load_and_start_image_via_tfa(__dtb_z_imx8mm_hgs_gs05_start,
				(void *)(MX8M_DDR_CSD1_BASE_ADDR + OPTEE_SIZE));
}

/*
 * Power-on execution flow might not be obvious for a very first read,
 * so here's, hopefully helpful, summary:
 *
 * 1. MaskROM uploads PBL into OCRAM and that's where this function is
 *    executed for the first time. At entry the exception level is EL3.
 *
 * 2. DDR is initialized and the image is loaded from storage into DRAM. The PBL
 *    part is copied from OCRAM to the TF-A return address in DRAM.
 *
 * 3. TF-A is executed and exits into the PBL code in DRAM. TF-A has taken us
 *    from EL3 to EL2.
 *
 * 4. Standard barebox boot flow continues
 */
static __noreturn noinline void hgs_gs05_start(void)
{
	hgs_early_hw_init(HGS_HW_GS05);

	start_atf();

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mm_barebox_entry(__dtb_z_imx8mm_hgs_gs05_start);
}

ENTRY_FUNCTION(start_hgs_gs05, r0, r1, r2)
{
	imx8mm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	hgs_gs05_start();
}
