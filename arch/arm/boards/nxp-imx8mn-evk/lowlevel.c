// SPDX-License-Identifier: GPL-2.0

#include <io.h>
#include <image-metadata.h>
#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <firmware.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <linux/sizes.h>
#include <mach/imx/atf.h>
#include <mach/imx/xload.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8mn-regs.h>
#include <mach/imx/iomux-mx8mn.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mfd/pca9450.h>
#include <mfd/bd71837.h>
#include <soc/imx8m/ddr.h>

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mn_setup_pad(IMX8MN_PAD_UART2_TXD__UART2_DCE_TX);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

static struct pmic_config pca9450_cfg[] = {
	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	{ PCA9450_BUCK123_DVS, 0x29 },
	/*
	 * increase VDD_SOC to typical value 0.95V before first
	 * DRAM access, set DVS1 to 0.85v for suspend.
	 * Enable DVS control through PMIC_STBY_REQ and
	 * set B1_ENMODE=1 (ON by PMIC_ON_REQ=H)
	 */
	{ PCA9450_BUCK1OUT_DVS0, 0x1C },
	/* Set DVS1 to 0.85v for suspend */
	/* Enable DVS control through PMIC_STBY_REQ and set B1_ENMODE=1 (ON by PMIC_ON_REQ=H) */
	{ PCA9450_BUCK1OUT_DVS1, 0x14 },
	{ PCA9450_BUCK1CTRL, 0x59 },
	/* set VDD_SNVS_0V8 from default 0.85V */
	{ PCA9450_LDO2CTRL, 0xC0 },
	/* enable LDO4 to 1.2v */
	{ PCA9450_LDO4CTRL, 0x44 },
	/* set WDOG_B_CFG to cold reset */
	{ PCA9450_RESET_CTRL, 0xA1 },
};

static struct pmic_config bd71837_cfg[] = {
	/* decrease RESET key long push time from the default 10s to 10ms */
	{ BD718XX_PWRONCONFIG1, 0x0 },
	/* unlock the PMIC regs */
	{ BD718XX_REGLOCK, 0x1 },
	/* Set VDD_ARM to typical value 0.85v for 1.2Ghz */
	{ BD718XX_BUCK2_VOLT_RUN, 0xf },
	/* Set VDD_SOC/VDD_DRAM to typical value 0.85v for nominal mode */
	{ BD718XX_BUCK1_VOLT_RUN, 0xf },
	/* Set VDD_SOC 0.85v for suspend */
	{ BD718XX_BUCK1_VOLT_SUSP, 0xf },
	/* increase NVCC_DRAM_1V2 to 1.2v for DDR4 */
	{ BD718XX_4TH_NODVS_BUCK_CTRL, 0x28 },
	/* lock the PMIC regs */
	{ BD718XX_REGLOCK, 0x11 },
};

extern struct dram_timing_info imx8mn_evk_ddr4_timing, imx8mn_evk_lpddr4_timing;

static void start_atf(void)
{
	struct pbl_i2c *i2c;

	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	imx8mn_early_clock_init();

	imx8mn_setup_pad(IMX8MN_PAD_I2C1_SCL__I2C1_SCL);
	imx8mn_setup_pad(IMX8MN_PAD_I2C1_SDA__I2C1_SDA);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MN_I2C1_BASE_ADDR));

	if (i2c_dev_probe(i2c, 0x25, true) == 0) {
		pmic_configure(i2c, 0x25, pca9450_cfg, ARRAY_SIZE(pca9450_cfg));
		imx8mn_ddr_init(&imx8mn_evk_lpddr4_timing, DRAM_TYPE_LPDDR4);
	} else {
		pmic_configure(i2c, 0x4b, bd71837_cfg, ARRAY_SIZE(bd71837_cfg));
		imx8mn_ddr_init(&imx8mn_evk_ddr4_timing, DRAM_TYPE_DDR4);
	}

	imx8mn_load_and_start_image_via_tfa();
}

/*
 * Power-on execution flow of start_nxp_imx8mn_evk() might not be
 * obvious for a very first read, so here's, hopefully helpful,
 * summary:
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
static __noreturn noinline void nxp_imx8mn_evk_start(void)
{
	extern char __dtb_z_imx8mn_evk_start[], __dtb_z_imx8mn_ddr4_evk_start[];
	void *fdt;

	setup_uart();

	start_atf();

	/* Check if we configured DDR4 in EL3 */
	if (readl(MX8M_DDRC_CTL_BASE_ADDR) & BIT(4))
		fdt = __dtb_z_imx8mn_ddr4_evk_start;
	else
		fdt = __dtb_z_imx8mn_evk_start;

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mn_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_nxp_imx8mn_evk, r0, r1, r2)
{
	imx8mn_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	nxp_imx8mn_evk_start();
}
