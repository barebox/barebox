/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <bootsource.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/gpio.h>
#include <asm/armlinux.h>
#include <partition.h>
#include <notifier.h>
#include <fs.h>
#include <led.h>
#include <fcntl.h>
#include <nand.h>
#include <usb/ulpi.h>
#include <usb/chipidea-imx.h>
#include <spi/spi.h>
#include <mfd/mc13xxx.h>
#include <mfd/mc13892.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/imx5.h>
#include <mach/bbu.h>
#include <mach/iomux-mx51.h>
#include <mach/imx51-regs.h>
#include <mach/devices-imx51.h>
#include <mach/imx-flash-header.h>
#include <mach/revision.h>

#define GPIO_EFIKA_SDHC1_WP	IMX_GPIO_NR(1, 1)
#define GPIO_EFIKAMX_SDHC1_CD	IMX_GPIO_NR(1, 0)
#define GPIO_EFIKASB_SDHC1_CD	IMX_GPIO_NR(2, 27)
#define GPIO_EFIKASB_SDHC2_CD	IMX_GPIO_NR(1, 8)
#define GPIO_EFIKASB_SDHC2_WP	IMX_GPIO_NR(1, 7)

#define GPIO_BACKLIGHT_POWER	IMX_GPIO_NR(4, 12)
#define GPIO_BACKLIGHT_PWM	IMX_GPIO_NR(1, 2)
#define GPIO_LVDS_POWER		IMX_GPIO_NR(3, 7)
#define GPIO_LVDS_RESET		IMX_GPIO_NR(3, 5)
#define GPIO_LVDS_ENABLE	IMX_GPIO_NR(3, 12)
#define GPIO_LCD_ENABLE		IMX_GPIO_NR(3, 13)

#define GPIO_BLUETOOTH		IMX_GPIO_NR(2, 11)
#define GPIO_WIFI_ENABLE	IMX_GPIO_NR(2, 16)
#define GPIO_WIFI_RESET		IMX_GPIO_NR(2, 10)
#define GPIO_HUB_RESET		IMX_GPIO_NR(1, 5)
#define GPIO_SMSC3317_RESET	IMX_GPIO_NR(2, 9)

static iomux_v3_cfg_t efika_pads[] = {
	/* ECSPI1 */
	MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
	MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
	MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,
	MX51_PAD_CSPI1_SS0__GPIO4_24,
	MX51_PAD_CSPI1_SS1__GPIO4_25,
	MX51_PAD_GPIO1_6__GPIO1_6,

	/* ESDHC1 */
	MX51_PAD_SD1_CMD__SD1_CMD,
	MX51_PAD_SD1_CLK__SD1_CLK,
	MX51_PAD_SD1_DATA0__SD1_DATA0,
	MX51_PAD_SD1_DATA1__SD1_DATA1,
	MX51_PAD_SD1_DATA2__SD1_DATA2,
	MX51_PAD_SD1_DATA3__SD1_DATA3,
	MX51_PAD_GPIO1_1__GPIO1_1,

	/* USB HOST1 */
	MX51_PAD_USBH1_CLK__USBH1_CLK,
	MX51_PAD_USBH1_DIR__USBH1_DIR,
	MX51_PAD_USBH1_NXT__USBH1_NXT,
	MX51_PAD_USBH1_DATA0__USBH1_DATA0,
	MX51_PAD_USBH1_DATA1__USBH1_DATA1,
	MX51_PAD_USBH1_DATA2__USBH1_DATA2,
	MX51_PAD_USBH1_DATA3__USBH1_DATA3,
	MX51_PAD_USBH1_DATA4__USBH1_DATA4,
	MX51_PAD_USBH1_DATA5__USBH1_DATA5,
	MX51_PAD_USBH1_DATA6__USBH1_DATA6,
	MX51_PAD_USBH1_DATA7__USBH1_DATA7,
	MX51_PAD_USBH1_STP__GPIO1_27,
	MX51_PAD_EIM_A16__GPIO2_10,

	/* USB HOST2 */
	MX51_PAD_EIM_D27__GPIO2_9,
	MX51_PAD_GPIO1_5__GPIO1_5,
	MX51_PAD_EIM_D16__USBH2_DATA0,
	MX51_PAD_EIM_D17__USBH2_DATA1,
	MX51_PAD_EIM_D18__USBH2_DATA2,
	MX51_PAD_EIM_D19__USBH2_DATA3,
	MX51_PAD_EIM_D20__USBH2_DATA4,
	MX51_PAD_EIM_D21__USBH2_DATA5,
	MX51_PAD_EIM_D22__USBH2_DATA6,
	MX51_PAD_EIM_D23__USBH2_DATA7,
	MX51_PAD_EIM_A24__USBH2_CLK,
	MX51_PAD_EIM_A25__USBH2_DIR,
	MX51_PAD_EIM_A26__GPIO2_20,
	MX51_PAD_EIM_A27__USBH2_NXT,

	/* PATA */
	MX51_PAD_NANDF_WE_B__PATA_DIOW,
	MX51_PAD_NANDF_RE_B__PATA_DIOR,
	MX51_PAD_NANDF_ALE__PATA_BUFFER_EN,
	MX51_PAD_NANDF_CLE__PATA_RESET_B,
	MX51_PAD_NANDF_WP_B__PATA_DMACK,
	MX51_PAD_NANDF_RB0__PATA_DMARQ,
	MX51_PAD_NANDF_RB1__PATA_IORDY,
	MX51_PAD_GPIO_NAND__PATA_INTRQ,
	MX51_PAD_NANDF_CS2__PATA_CS_0,
	MX51_PAD_NANDF_CS3__PATA_CS_1,
	MX51_PAD_NANDF_CS4__PATA_DA_0,
	MX51_PAD_NANDF_CS5__PATA_DA_1,
	MX51_PAD_NANDF_CS6__PATA_DA_2,
	MX51_PAD_NANDF_D15__PATA_DATA15,
	MX51_PAD_NANDF_D14__PATA_DATA14,
	MX51_PAD_NANDF_D13__PATA_DATA13,
	MX51_PAD_NANDF_D12__PATA_DATA12,
	MX51_PAD_NANDF_D11__PATA_DATA11,
	MX51_PAD_NANDF_D10__PATA_DATA10,
	MX51_PAD_NANDF_D9__PATA_DATA9,
	MX51_PAD_NANDF_D8__PATA_DATA8,
	MX51_PAD_NANDF_D7__PATA_DATA7,
	MX51_PAD_NANDF_D6__PATA_DATA6,
	MX51_PAD_NANDF_D5__PATA_DATA5,
	MX51_PAD_NANDF_D4__PATA_DATA4,
	MX51_PAD_NANDF_D3__PATA_DATA3,
	MX51_PAD_NANDF_D2__PATA_DATA2,
	MX51_PAD_NANDF_D1__PATA_DATA1,
	MX51_PAD_NANDF_D0__PATA_DATA0,

	MX51_PAD_EIM_A22__GPIO2_16, /* WLAN enable (1 = on) */
	MX51_PAD_EIM_A17__GPIO2_11,

	/* I2C2 */
	MX51_PAD_KEY_COL4__I2C2_SCL,
	MX51_PAD_KEY_COL5__I2C2_SDA,

	MX51_PAD_GPIO1_2__GPIO1_2, /* Backlight (should be pwm) (1 = on) */
	MX51_PAD_CSI2_D19__GPIO4_12, /* Backlight power (0 = on) */

	MX51_PAD_DISPB2_SER_CLK__GPIO3_7, /* LVDS power (1 = on) */
	MX51_PAD_DISPB2_SER_DIN__GPIO3_5, /* LVDS reset (1 = reset) */
	MX51_PAD_CSI1_D8__GPIO3_12, /* LVDS enable (1 = enable) */
	MX51_PAD_CSI1_D9__GPIO3_13, /* LCD enable (1 = on) */

	MX51_PAD_DI1_PIN12__GPIO3_1, /* WLAN switch (0 = on) */

	MX51_PAD_GPIO1_4__WDOG1_WDOG_B,
};

static iomux_v3_cfg_t efikasb_pads[] = {
	/* LEDs */
	MX51_PAD_EIM_CS0__GPIO2_25,
	MX51_PAD_GPIO1_3__GPIO1_3,

	/* ESHC2 */
	MX51_PAD_SD2_CMD__SD2_CMD,
	MX51_PAD_SD2_CLK__SD2_CLK,
	MX51_PAD_SD2_DATA0__SD2_DATA0,
	MX51_PAD_SD2_DATA1__SD2_DATA1,
	MX51_PAD_SD2_DATA2__SD2_DATA2,
	MX51_PAD_SD2_DATA3__SD2_DATA3,
	MX51_PAD_GPIO1_7__GPIO1_7,
	MX51_PAD_GPIO1_8__GPIO1_8,

	MX51_PAD_EIM_CS2__GPIO2_27,
};

static iomux_v3_cfg_t efikamx_pads[] = {
	MX51_PAD_GPIO1_0__GPIO1_0,
};

/*
 * Generally this should work on the Efika MX smarttop aswell,
 * but I do not have the hardware to test it, so hardcode this
 * for the smartbook for now.
 */
static inline int machine_is_efikasb(void)
{
	return 1;
}

static int efikamx_mem_init(void)
{
	arm_add_mem_device("ram0", 0x90000000, SZ_512M);

	return 0;
}
mem_initcall(efikamx_mem_init);

static int spi_0_cs[] = { IMX_GPIO_NR(4, 24), IMX_GPIO_NR(4, 25) };

static struct spi_imx_master spi_0_data = {
	.chipselect = spi_0_cs,
	.num_chipselect = ARRAY_SIZE(spi_0_cs),
};

static const struct spi_board_info efikamx_spi_board_info[] = {
	{
		.name = "mc13xxx-spi",
		.max_speed_hz = 30 * 1000 * 1000,
		.bus_num = 0,
		.chip_select = 0,
	}, {
		.name = "m25p80",
		.chip_select = 1,
		.max_speed_hz = 20 * 1000 * 1000,
		.bus_num = 0,
	},
};

static void efikamx_power_init(void)
{
	unsigned int val;
	struct mc13xxx *mc;

	mc = mc13xxx_get();
	if (!mc) {
		printf("could not get mc13892\n");
		return;
	}

	/* Write needed to Power Gate 2 register */
	mc13xxx_reg_read(mc, MC13892_REG_POWER_MISC, &val);
	val &= ~MC13892_POWER_MISC_PWGT2SPIEN;
	mc13xxx_reg_write(mc, MC13892_REG_POWER_MISC, val);

	/* Externally powered */
	mc13xxx_reg_read(mc, MC13892_REG_CHARGE, &val);
	val |= MC13782_CHARGE_ICHRG0 | MC13782_CHARGE_ICHRG1 |
		MC13782_CHARGE_ICHRG2 | MC13782_CHARGE_ICHRG3 |
		MC13782_CHARGE_CHGAUTOB;
	mc13xxx_reg_write(mc, MC13892_REG_CHARGE, val);

	/* power up the system first */
	mc13xxx_reg_write(mc, MC13892_REG_POWER_MISC,
			MC13892_POWER_MISC_PWUP);

	/* Set core voltage to 1.1V */
	mc13xxx_reg_read(mc, MC13892_REG_SW_0, &val);
	val &= ~MC13892_SWx_SWx_VOLT_MASK;
	val |= MC13892_SWx_SWx_1_100V;
	mc13xxx_reg_write(mc, MC13892_REG_SW_0, val);

	/* Setup VCC (SW2) to 1.25 */
	mc13xxx_reg_read(mc, MC13892_REG_SW_1, &val);
	val &= ~MC13892_SWx_SWx_VOLT_MASK;
	val |= MC13892_SWx_SWx_1_250V;
	mc13xxx_reg_write(mc, MC13892_REG_SW_1, val);

	/* Setup 1V2_DIG1 (SW3) to 1.25 */
	mc13xxx_reg_read(mc, MC13892_REG_SW_2, &val);
	val &= ~MC13892_SWx_SWx_VOLT_MASK;
	val |= MC13892_SWx_SWx_1_250V;
	mc13xxx_reg_write(mc, MC13892_REG_SW_2, val);
	udelay(50);

	/* Raise the core frequency to 800MHz */
	console_flush();
	imx51_init_lowlevel(800);
	clock_notifier_call_chain();

	/* Set switchers in Auto in NORMAL mode & STANDBY mode */
	/* Setup the switcher mode for SW1 & SW2*/
	mc13xxx_reg_read(mc, MC13892_REG_SW_4, &val);
	val = (val & ~((MC13892_SWMODE_MASK << MC13892_SWMODE1_SHIFT) |
			(MC13892_SWMODE_MASK << MC13892_SWMODE2_SHIFT)));
	val |= (MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE1_SHIFT) |
		(MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE2_SHIFT);
	mc13xxx_reg_write(mc, MC13892_REG_SW_4, val);

	/* Setup the switcher mode for SW3 & SW4 */
	mc13xxx_reg_read(mc, MC13892_REG_SW_5, &val);
	val = (val & ~((MC13892_SWMODE_MASK << MC13892_SWMODE3_SHIFT) |
			(MC13892_SWMODE_MASK << MC13892_SWMODE4_SHIFT)));
	val |= (MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE3_SHIFT) |
		(MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE4_SHIFT);
	mc13xxx_reg_write(mc, MC13892_REG_SW_5, val);

	/* Set VDIG to 1.8V, VGEN3 to 1.8V, VCAM to 2.6V */
	mc13xxx_reg_read(mc, MC13892_REG_SETTING_0, &val);
	val &= ~(MC13892_SETTING_0_VCAM_MASK |
		MC13892_SETTING_0_VGEN3_MASK |
		MC13892_SETTING_0_VDIG_MASK);
	val |= MC13892_SETTING_0_VDIG_1_8 |
		MC13892_SETTING_0_VGEN3_1_8 |
		MC13892_SETTING_0_VCAM_2_6;
	mc13xxx_reg_write(mc, MC13892_REG_SETTING_0, val);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13xxx_reg_read(mc, MC13892_REG_SETTING_1, &val);
	val &= ~(MC13892_SETTING_1_VVIDEO_MASK |
			MC13892_SETTING_1_VSD_MASK |
			MC13892_SETTING_1_VAUDIO_MASK);
	val |= MC13892_SETTING_1_VSD_3_15 |
		MC13892_SETTING_1_VAUDIO_3_0 |
		MC13892_SETTING_1_VVIDEO_2_775 |
		MC13892_SETTING_1_VGEN1_1_2 |
		MC13892_SETTING_1_VGEN2_3_15;
	mc13xxx_reg_write(mc, MC13892_REG_SETTING_1, val);

	/* Enable VGEN1, VGEN2, VDIG, VPLL */
	mc13xxx_reg_read(mc, MC13892_REG_MODE_0, &val);
	val |= MC13892_MODE_0_VGEN1EN |
		MC13892_MODE_0_VDIGEN |
		MC13892_MODE_0_VGEN2EN |
		MC13892_MODE_0_VPLLEN;
	mc13xxx_reg_write(mc, MC13892_REG_MODE_0, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = MC13892_MODE_1_VGEN3CONFIG |
		MC13892_MODE_1_VCAMCONFIG;
	mc13xxx_reg_write(mc, MC13892_REG_MODE_1, val);
	udelay(200);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = MC13892_MODE_1_VGEN3EN |
		MC13892_MODE_1_VGEN3CONFIG |
		MC13892_MODE_1_VCAMEN |
		MC13892_MODE_1_VCAMCONFIG |
		MC13892_MODE_1_VVIDEOEN |
		MC13892_MODE_1_VAUDIOEN |
		MC13892_MODE_1_VSDEN;
	mc13xxx_reg_write(mc, MC13892_REG_MODE_1, val);

	mc13xxx_reg_read(mc, MC13892_REG_POWER_CTL2, &val);
	val |= MC13892_POWER_CONTROL_2_WDIRESET;
	mc13xxx_reg_write(mc, MC13892_REG_POWER_CTL2, val);

	udelay(2500);
}

static struct esdhc_platform_data efikasb_sd2_data = {
	.cd_gpio = GPIO_EFIKASB_SDHC2_CD,
	.wp_gpio = GPIO_EFIKASB_SDHC2_WP,
	.cd_type = ESDHC_CD_GPIO,
	.wp_type = ESDHC_WP_GPIO,
	.devname = "mmc_left",
};

static struct esdhc_platform_data efikamx_sd1_data = {
	.cd_gpio = GPIO_EFIKAMX_SDHC1_CD,
	.wp_gpio = GPIO_EFIKA_SDHC1_WP,
	.cd_type = ESDHC_CD_GPIO,
	.wp_type = ESDHC_WP_GPIO,
};

static struct esdhc_platform_data efikasb_sd1_data = {
	.cd_gpio = GPIO_EFIKASB_SDHC1_CD,
	.wp_gpio = GPIO_EFIKA_SDHC1_WP,
	.cd_type = ESDHC_CD_GPIO,
	.wp_type = ESDHC_WP_GPIO,
	.devname = "mmc_back",
};

struct imxusb_platformdata efikamx_usbh1_pdata = {
	.flags = MXC_EHCI_MODE_ULPI | MXC_EHCI_INTERFACE_DIFF_UNI,
	.mode = IMX_USB_MODE_HOST,
};

static int efikamx_usb_init(void)
{
	gpio_direction_output(GPIO_BLUETOOTH, 0);
	gpio_direction_output(GPIO_WIFI_ENABLE, 1);
	gpio_direction_output(GPIO_WIFI_RESET, 0);
	gpio_direction_output(GPIO_SMSC3317_RESET, 0);
	gpio_direction_output(GPIO_HUB_RESET, 0);

	mdelay(10);

	gpio_set_value(GPIO_HUB_RESET, 1);
	gpio_set_value(GPIO_SMSC3317_RESET, 1);
	gpio_set_value(GPIO_BLUETOOTH, 1);
	gpio_set_value(GPIO_WIFI_RESET, 1);

	mxc_iomux_v3_setup_pad(MX51_PAD_USBH1_STP__GPIO1_27);
	gpio_set_value(IMX_GPIO_NR(1, 27), 1);
	mdelay(1);
	mxc_iomux_v3_setup_pad(MX51_PAD_USBH1_STP__USBH1_STP);

	if (machine_is_efikasb()) {
		mxc_iomux_v3_setup_pad(MX51_PAD_EIM_A26__GPIO2_20);
		gpio_set_value(IMX_GPIO_NR(2, 20), 1);
		mdelay(1);
		mxc_iomux_v3_setup_pad(MX51_PAD_EIM_A26__USBH2_STP);
	}

	imx51_add_usbh1(&efikamx_usbh1_pdata);

	/*
	 * At least for the EfikaSB these do not seem to be interesting.
	 * The external ports are all connected to host1.
	 *
	 * imx51_add_usbotg(pdata);
	 * imx51_add_usbh2(pdate);
	 */

	return 0;
}

static struct gpio_led leds[] = {
	{
		.gpio = IMX_GPIO_NR(1, 3),
		.active_low = 1,
		.led.name = "mail",
	}, {
		.gpio = IMX_GPIO_NR(2, 25),
		.led.name = "white",
	},
};

#define DCD_NAME static struct imx_dcd_entry dcd_entry

#include "dcd-data.h"

static int efikamx_devices_init(void)
{
	int i;

	mxc_iomux_v3_setup_multiple_pads(efika_pads, ARRAY_SIZE(efika_pads));
	if (machine_is_efikasb()) {
		gpio_direction_output(GPIO_BACKLIGHT_POWER, 1);
		mxc_iomux_v3_setup_multiple_pads(efikasb_pads,
				ARRAY_SIZE(efikasb_pads));
	} else {
		mxc_iomux_v3_setup_multiple_pads(efikamx_pads,
				ARRAY_SIZE(efikamx_pads));
	}

	spi_register_board_info(efikamx_spi_board_info,
			ARRAY_SIZE(efikamx_spi_board_info));
	imx51_add_spi0(&spi_0_data);

	efikamx_power_init();

	if (machine_is_efikasb())
		imx51_add_mmc0(&efikasb_sd1_data);
	else
		imx51_add_mmc0(&efikamx_sd1_data);

	imx51_add_mmc1(&efikasb_sd2_data);

	for (i = 0; i < ARRAY_SIZE(leds); i++)
		led_gpio_register(&leds[i]);

	imx51_add_i2c1(NULL);

	efikamx_usb_init();

	imx51_add_pata();

	writew(0x0, MX51_WDOG_BASE_ADDR + 0x8);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc_left",
			BBU_HANDLER_FLAG_DEFAULT, dcd_entry, sizeof(dcd_entry),
			0);

	armlinux_set_bootparams((void *)0x90000100);
	armlinux_set_architecture(2370);
	armlinux_set_revision(0x5100 | imx_silicon_revision());

	return 0;
}
device_initcall(efikamx_devices_init);

static int efikamx_part_init(void)
{
	if (bootsource_get() == BOOTSOURCE_MMC) {
		devfs_add_partition("mmc_left", 0x00000, 0x80000,
				DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("mmc_left", 0x80000, 0x80000,
				DEVFS_PARTITION_FIXED, "env0");
	}

	return 0;
}
late_initcall(efikamx_part_init);

static iomux_v3_cfg_t efika_uart_pads[] = {
	/* UART */
	MX51_PAD_UART1_RXD__UART1_RXD,
	MX51_PAD_UART1_TXD__UART1_TXD,
	MX51_PAD_UART1_RTS__UART1_RTS,
	MX51_PAD_UART1_CTS__UART1_CTS,
};

static int efikamx_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(efika_uart_pads,
			ARRAY_SIZE(efika_uart_pads));

	imx51_add_uart0();

	return 0;
}
console_initcall(efikamx_console_init);
