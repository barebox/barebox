// SPDX-License-Identifier: GPL-2.0

#include <io.h>
#include <common.h>
#include <firmware.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <linux/sizes.h>
#include <mach/imx/atf.h>
#include <mach/imx/xload.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8mp-regs.h>
#include <mach/imx/iomux-mx8mp.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/debug_ll.h>
#include <mfd/pca9450.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <soc/imx8m/ddr.h>
#include <boards/tq/tq_eeprom.h>

#define UART_PAD_CTRL   MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_FSEL)

#define I2C_PAD_CTRL	MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_HYS | \
				     MX8MP_PAD_CTL_PUE | \
				     MX8MP_PAD_CTL_PE)


extern struct dram_timing_info *dram_timings_xs_no_ecc[];
extern struct dram_timing_info *dram_timings_xl_no_ecc[];

static void setup_uart(void *uart, iomux_v3_cfg_t tx_pad, iomux_v3_cfg_t rx_pad)
{
	imx8m_early_setup_uart_clock();

	imx8mp_setup_pad(tx_pad | UART_PAD_CTRL);
	imx8mp_setup_pad(rx_pad | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

static struct pbl_i2c *tqma_i2c1_init(void)
{
	imx8mp_setup_pad(MX8MP_PAD_I2C1_SCL__I2C1_SCL | I2C_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_I2C1_SDA__I2C1_SDA | I2C_PAD_CTRL);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	return imx8m_i2c_early_init(IOMEM(MX8MP_I2C1_BASE_ADDR));
}

static struct pbl_i2c *tqma_i2c2_init(void)
{
	imx8mp_setup_pad(MX8MP_PAD_I2C2_SCL__I2C2_SCL | I2C_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_I2C2_SDA__I2C2_SDA | I2C_PAD_CTRL);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C2);

	return imx8m_i2c_early_init(IOMEM(MX8MP_I2C2_BASE_ADDR));
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
	{ PCA9450_BUCK1OUT_DVS1, 0x14 },
	{ PCA9450_BUCK1CTRL, 0x59 },
	/*
	 * Kernel uses OD/OD freq for SOC
	 * To avoid timing risk from SOC to ARM,increase VDD_ARM to OD
	 * voltage 0.95v
	 */
	{ PCA9450_BUCK2OUT_DVS0, 0x1C },
	/* set WDOG_B_CFG to cold reset */
	{ PCA9450_RESET_CTRL, 0xA1 },
};

static bool tqma_is_eeprom_valid(struct tq_eeprom *eeprom)
{
	int ramsize;

	if (!*eeprom->serial || !*eeprom->id)
		return false;
	ramsize = tq_vard_ramsize(&eeprom->vard) / (SZ_1G);
	if (ramsize != 1 && ramsize != 2 && ramsize != 4 && ramsize != 8)
		return false;
	return true;
}

static noinline void tqma8mpxx_start(void)
{
	extern char __dtb_z_imx8mp_tqma8mpql_mba8mpxl_start[];
	extern char __dtb_z_imx8mp_tqma8mpqs_mba8mpxs_start[];
	struct dram_timing_info **dram_timings;

	struct tq_eeprom *eeprom;
	struct pbl_i2c *i2c;
	void *boarddata;

	i2c = tqma_i2c1_init();

	/**
	 * The difference for the lowlevel code between xS and xL is:
	 * PMIC: xS on i2c2, xL on i2C1
	 * VARD: address 0x50 on xS, address 0x53 on xL.
	 *       offset 0x1000 on xS, offset 0x0 on xL
	 */

	eeprom = pbl_tq_read_eeprom(i2c, 0x50, 0x1000 | I2C_ADDR_16_BIT);
	if (tqma_is_eeprom_valid(eeprom)) {
		/* found xS board */
		i2c = tqma_i2c2_init();
		boarddata = __dtb_z_imx8mp_tqma8mpqs_mba8mpxs_start;
		dram_timings = dram_timings_xs_no_ecc;
		setup_uart(IOMEM(MX8M_UART3_BASE_ADDR),
			   MX8MP_PAD_SD1_DATA6__UART3_DCE_TX,
			   MX8MP_PAD_SD1_DATA7__UART3_DCE_RX);
	} else {
		eeprom = pbl_tq_read_eeprom(i2c, 0x53, 0);
		if (!tqma_is_eeprom_valid(eeprom))
			panic("Could not read VARD!\n");

		/* found xL board */
		boarddata = __dtb_z_imx8mp_tqma8mpql_mba8mpxl_start;
		dram_timings = dram_timings_xl_no_ecc;
		setup_uart(IOMEM(MX8M_UART4_BASE_ADDR),
			   MX8MP_PAD_UART4_TXD__UART4_DCE_TX,
			   MX8MP_PAD_UART4_RXD__UART4_DCE_RX);

	}

	if (current_el() == 3) {
		unsigned long ramsize;
		int index = -1;

		ramsize = tq_vard_ramsize(&eeprom->vard);
		switch (ramsize) {
		case SZ_1G:
			index = 0;
			break;
		case SZ_2G:
			index = 1;
			break;
		case SZ_4G:
			index = 2;
			break;
		case SZ_8G:
			index = 3;
			break;
		default:
			panic("RAMsize %lu is not supported.\n", ramsize);
		}

		imx8mp_early_clock_init();

		pmic_configure(i2c, 0x25, pca9450_cfg, ARRAY_SIZE(pca9450_cfg));

		imx8mp_ddr_init(dram_timings[index], DRAM_TYPE_LPDDR4);

		imx8mp_load_and_start_image_via_tfa();
	}

	tq_vard_show(&eeprom->vard);
	printf("Serial: %s\n", eeprom->id);
	printf("ID:     %s\n", eeprom->serial);

	if (tq_vard_has_ramecc(&eeprom->vard))
		pr_err("ECC Configured, but treated as non ECC RAM\n");

	imx8mp_barebox_entry(boarddata);
}

ENTRY_FUNCTION(start_tqma8mpxx, x0, x1, x2)
{
	imx8mp_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	tqma8mpxx_start();
}
