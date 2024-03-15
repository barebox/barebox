// SPDX-License-Identifier: GPL-2.0

#include <asm/barebox-arm.h>
#include <common.h>
#include <debug_ll.h>
#include <mach/imx/atf.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/iomux-mx8mp.h>
#include <mach/imx/xload.h>
#include <mfd/pca9450.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <soc/imx8m/ddr.h>

#include "lowlevel.h"

extern char __dtb_z_imx8mp_karo_qsxp_ml81_qsbase4_start[];

#define UART_PAD_CTRL	MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_FSEL)

#define I2C_PAD_CTRL	MUX_PAD_CTRL(MX8MP_PAD_CTL_PE | \
				     MX8MP_PAD_CTL_HYS | \
				     MX8MP_PAD_CTL_PUE | \
				     MX8MP_PAD_CTL_DSE6)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mp_setup_pad(MX8MP_PAD_UART2_TXD__UART2_DCE_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

#define pca9450_mV_to_reg(mV)	(((mV) - 600) * 10 / 125)
#define pca9450_reg_to_mV(val)	(((val) * 125 / 10) + 600)

#define VDD_SOC_VAL	pca9450_mV_to_reg(950)
#define VDD_SOC_SLP_VAL	pca9450_mV_to_reg(850)
#define VDD_ARM_VAL	pca9450_mV_to_reg(950)
#define VDD_DRAM_VAL	pca9450_mV_to_reg(950)

static struct pmic_config pca9450_cfg[] = {
	{ PCA9450_BUCK123_DVS, 0x29 },
	{ PCA9450_BUCK1OUT_DVS0, VDD_SOC_VAL },
	{ PCA9450_BUCK1OUT_DVS1, VDD_SOC_SLP_VAL },
	{ PCA9450_BUCK2OUT_DVS0, VDD_ARM_VAL },
	{ PCA9450_BUCK3OUT_DVS0, VDD_DRAM_VAL },
	{ PCA9450_BUCK1CTRL, 0x59 },
	{ PCA9450_RESET_CTRL, 0xA1 },
};

static void power_init_board(void)
{
	struct pbl_i2c *i2c;

	imx8mp_setup_pad(MX8MP_PAD_I2C1_SCL__I2C1_SCL | I2C_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_I2C1_SDA__I2C1_SDA | I2C_PAD_CTRL);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MP_I2C1_BASE_ADDR));

	pmic_configure(i2c, 0x25, pca9450_cfg, ARRAY_SIZE(pca9450_cfg));
}

ENTRY_FUNCTION(start_karo_qsxp_ml81, r0, r1, r2)
{
	imx8mp_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	setup_uart();

	/*
	 * If we are in EL3 we are running for the first time out of OCRAM,
	 * we'll need to initialize the DRAM and run TF-A (BL31). The TF-A
	 * will then jump to DRAM in EL2
	 */
	if (current_el() == 3) {
		imx8mp_early_clock_init();

		power_init_board();

		imx8mp_ddr_init(&karo_qsxp_ml81_dram_timing, DRAM_TYPE_LPDDR4);

		imx8mp_load_and_start_image_via_tfa();
	}

	/* Standard entry we hit once we initialized both DDR and ATF */
	imx8mp_barebox_entry(__dtb_z_imx8mp_karo_qsxp_ml81_qsbase4_start);
}
