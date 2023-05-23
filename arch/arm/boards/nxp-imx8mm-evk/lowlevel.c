// SPDX-License-Identifier: GPL-2.0

#include <io.h>
#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <linux/sizes.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8mm-regs.h>
#include <mach/imx/iomux-mx8mm.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mfd/bd71837.h>
#include <mfd/pca9450.h>
#include <mach/imx/xload.h>
#include <soc/imx8m/ddr.h>

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mm_setup_pad(IMX8MM_PAD_UART2_TXD_UART2_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

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
	 * VDD_DRAM needs off in suspend, set B1_ENMODE=10
	 * (ON by PMIC_ON_REQ = H && PMIC_STBY_REQ = L)
	 */
	{ PCA9450_BUCK3CTRL, 0x4a },

	/* set VDD_SNVS_0V8 from default 0.85V */
	{ PCA9450_LDO2CTRL, 0xc0 },

	/* set WDOG_B_CFG to cold reset */
	{ PCA9450_RESET_CTRL, 0xa1 },
};

static struct pmic_config bd71837_cfg[] = {
	/* decrease RESET key long push time from the default 10s to 10ms */
	{ BD718XX_PWRONCONFIG1, 0x0 },
	/* unlock the PMIC regs */
	{ BD718XX_REGLOCK, 0x1 },
	/* increase VDD_SOC to typical value 0.85v before first DRAM access */
	{ BD718XX_BUCK1_VOLT_RUN, 0x0f },
	/* increase VDD_DRAM to 0.975v for 3Ghz DDR */
	{ BD718XX_1ST_NODVS_BUCK_VOLT, 0x83 },
	/* lock the PMIC regs */
	{ BD718XX_REGLOCK, 0x11 },
};

static void power_init_board(void)
{
	struct pbl_i2c *i2c;

	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SCL_I2C1_SCL);
	imx8mm_setup_pad(IMX8MM_PAD_I2C1_SDA_I2C1_SDA);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MQ_I2C1_BASE_ADDR));

	if (i2c_dev_probe(i2c, 0x25, true) == 0)
		pmic_configure(i2c, 0x25, pca9450_cfg, ARRAY_SIZE(pca9450_cfg));
	else
		pmic_configure(i2c, 0x4b, bd71837_cfg, ARRAY_SIZE(bd71837_cfg));
}

extern struct dram_timing_info imx8mm_evk_dram_timing;

static void start_atf(void)
{
	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	imx8mm_early_clock_init();
	power_init_board();
	imx8mm_ddr_init(&imx8mm_evk_dram_timing, DRAM_TYPE_LPDDR4);

	imx8mm_load_and_start_image_via_tfa();
}

/*
 * Power-on execution flow of start_nxp_imx8mm_evk() might not be
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
static __noreturn noinline void nxp_imx8mm_evk_start(void)
{
	extern char __dtb_z_imx8mm_evk_start[], __dtb_z_imx8mm_evkb_start[];
	struct pbl_i2c *i2c;
	void *fdt;

	setup_uart();

	start_atf();

	/*
	 * Standard entry we hit once we initialized both DDR and ATF. I2C pad
	 * and clock setup already done during power_init_board().
	 */
	i2c = imx8m_i2c_early_init(IOMEM(MX8MQ_I2C1_BASE_ADDR));

	if (i2c_dev_probe(i2c, 0x25, true) == 0)
		fdt = __dtb_z_imx8mm_evkb_start;
	else
		fdt = __dtb_z_imx8mm_evk_start;

	imx8mm_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_nxp_imx8mm_evk, r0, r1, r2)
{
	imx8mm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	nxp_imx8mm_evk_start();
}
