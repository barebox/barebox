// SPDX-License-Identifier: GPL-2.0

#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/atf.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/imx8mp-regs.h>
#include <mach/imx/iomux-mx8mp.h>
#include <mach/imx/xload.h>
#include <mfd/pca9450.h>
#include <pbl/i2c.h>
#include <pbl/pmic.h>
#include <soc/imx8m/ddr.h>
#include <mach/imx/imx-gpio.h>

extern char __dtb_z_imx8mp_skov_start[];

#define PGOOD_PAD_CTRL  MUX_PAD_CTRL(MX8MP_PAD_CTL_PUE | \
				     MX8MP_PAD_CTL_PE)

#define UART_PAD_CTRL   MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_FSEL | \
				     MX8MP_PAD_CTL_PUE | \
				     MX8MP_PAD_CTL_PE)

#define I2C_PAD_CTRL	MUX_PAD_CTRL(MX8MP_PAD_CTL_DSE6 | \
				     MX8MP_PAD_CTL_HYS | \
				     MX8MP_PAD_CTL_PUE | \
				     MX8MP_PAD_CTL_PE)

static inline void led_d1_toggle(bool *on)
{
	imx8m_gpio_direction_output(IOMEM(MX8MP_GPIO1_BASE_ADDR), 5, *on);
	*on = !*on;
}

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART2_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mp_setup_pad(MX8MP_PAD_UART2_TXD__UART2_DCE_TX | UART_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_UART2_RXD__UART2_DCE_RX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putchar('>');
}

static struct pmic_config pca9450_cfg[] = {
	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	{ PCA9450_BUCK123_DVS, 0x29 },
	/*
	 * Set VDD_SOC to typical value 0.85V before first
	 * DRAM access, set DVS1 to 0.85v for suspend.
	 * Enable DVS control through PMIC_STBY_REQ and
	 * set B1_ENMODE=1 (ON by PMIC_ON_REQ=H)
	 */
	{ PCA9450_BUCK1OUT_DVS0, 0x14 },
	{ PCA9450_BUCK1OUT_DVS1, 0x14 },
	{ PCA9450_BUCK1CTRL, 0x59 },
	/* set WDOG_B_CFG to cold reset */
	{ PCA9450_RESET_CTRL, 0xA1 },
	/*
	 * As we do cold resets and Linux will take care to reconfigure the
	 * pmic before switching to the OD ARM frequency, we will just keep
	 * VDD_ARM at 850mV
	 */
	{ PCA9450_BUCK2OUT_DVS0, 0x14 },
};

static inline bool power_good(void)
{
	/* IMX_SHDN_MF in schematics */
	return imx8m_gpio_val(IOMEM(MX8MP_GPIO4_BASE_ADDR), 23);
}

static void wait_for_power_good(void)
{
	void __iomem *gpio4 = IOMEM(MX8MP_GPIO4_BASE_ADDR);
	int timeout_ms = 0;
	bool led_active = true;

	imx8mp_setup_pad(MX8MP_PAD_SAI2_RXD0__GPIO4_IO23 | PGOOD_PAD_CTRL);
	imx8m_gpio_direction_input(gpio4, 23);

	led_d1_toggle(&led_active);

	if (power_good())
		return;

	pr_warn("\nDelaying boot until power stabilizes\n");

	/* If we reach this, because Linux did a hw_protection_reboot, we don't
	 * want to continue booting right away.
	 *
	 * Thus let's either wait for the condition to subscede or for voltage
	 * to reach a low enough level for the PMIC to detect VSYS_UVLO going
	 * lower than allowed
	 */

	while (1) {
		if (power_good()) {
			/* wait 10ms longer and check if it still good */
			udelay(10000);
			if (power_good()) {
				pr_info("IMX_SHDN_MF stuck low for ~%ums.\n", timeout_ms);
				break;
			}
		}
		/* fast blink LED D1 */
		if (timeout_ms % 100 == 0) {
			pr_debug(".");
			led_d1_toggle(&led_active);
		}
		udelay(1000);
		timeout_ms++;
	}
}

static void power_init_board(void)
{
	struct pbl_i2c *i2c;

	/* Assert switch reset early to avoid erratic behavior due to
	 * violating power sequencing
	 */
	imx8mp_setup_pad(MX8MP_PAD_SAI3_TXD__GPIO5_IO01);
	imx8m_gpio_direction_output(IOMEM(MX8MP_GPIO5_BASE_ADDR), 1, 0);

	wait_for_power_good();

	imx8mp_setup_pad(MX8MP_PAD_I2C1_SCL__I2C1_SCL | I2C_PAD_CTRL);
	imx8mp_setup_pad(MX8MP_PAD_I2C1_SDA__I2C1_SDA | I2C_PAD_CTRL);

	imx8m_ccgr_clock_enable(IMX8M_CCM_CCGR_I2C1);

	i2c = imx8m_i2c_early_init(IOMEM(MX8MP_I2C1_BASE_ADDR));

	pmic_configure(i2c, 0x25, pca9450_cfg, ARRAY_SIZE(pca9450_cfg));
}

extern struct dram_timing_info imx8mp_skov_dram_timing;

static void start_atf(struct dram_timing_info *dram_timing)
{
	/*
	 * If we are in EL3 we are running for the first time and need to
	 * initialize the DRAM and run TF-A (BL31). The TF-A will then jump
	 * to DRAM in EL2.
	 */
	if (current_el() != 3)
		return;

	imx8mp_early_clock_init();

	power_init_board();

	imx8mp_ddr_init(dram_timing, DRAM_TYPE_LPDDR4);

	imx8mp_load_and_start_image_via_tfa();
}

/*
 * Power-on execution flow of imx8mp_skov_start() might not be
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
static __noreturn noinline void
imx8mp_skov_start(struct dram_timing_info *dram_timing, void *dtb)
{
	setup_uart();

	start_atf(dram_timing);

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mp_barebox_entry(dtb);
}

ENTRY_FUNCTION(start_skov_imx8mp, r0, r1, r2)
{
	imx8mp_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	imx8mp_skov_start(&imx8mp_skov_dram_timing,
			   __dtb_z_imx8mp_skov_start);
}
