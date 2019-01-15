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

#include <environment.h>
#include <bootsource.h>
#include <partition.h>
#include <common.h>
#include <envfs.h>
#include <fcntl.h>
#include <gpio.h>
#include <init.h>
#include <led.h>
#include <fs.h>
#include <io.h>
#include <of.h>

#include <spi/spi.h>
#include <mfd/mc13xxx.h>
#include <mfd/mc13892.h>

#include <asm/armlinux.h>

#include <mach/devices-imx51.h>
#include <mach/imx51-regs.h>
#include <mach/iomux-mx51.h>
#include <mach/revision.h>
#include <mach/generic.h>
#include <mach/imx5.h>
#include <mach/bbu.h>

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

/*
 * Generally this should work on the Efika MX smarttop aswell,
 * but I do not have the hardware to test it, so hardcode this
 * for the smartbook for now.
 */
static inline int machine_is_efikasb(void)
{
	return 1;
}

static void efikamx_power_init(struct mc13xxx *mc)
{
	unsigned int val;

	/* Write needed to Power Gate 2 register */
	mc13xxx_reg_read(mc, MC13892_REG_POWER_MISC, &val);
	val &= ~MC13892_POWER_MISC_PWGT2SPIEN;
	mc13xxx_reg_write(mc, MC13892_REG_POWER_MISC, val);

	/* Externally powered */
	mc13xxx_reg_read(mc, MC13892_REG_CHARGE, &val);
	val |= MC13782_CHARGE_ICHRG_FULL | MC13782_CHARGE_CHGAUTOB;
	mc13xxx_reg_write(mc, MC13892_REG_CHARGE, val);

	/* power up the system first */
	mc13xxx_reg_write(mc, MC13892_REG_POWER_MISC,
			  MC13892_POWER_MISC_GPO4ADIN);

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
		MC13892_SETTING_0_VGEN1_MASK |
		MC13892_SETTING_0_VGEN2_MASK |
		MC13892_SETTING_0_VGEN3_MASK |
		MC13892_SETTING_0_VDIG_MASK);
	val |= MC13892_SETTING_0_VDIG_1_8 |
		MC13892_SETTING_0_VGEN1_1_2 |
		MC13892_SETTING_0_VGEN2_3_15 |
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
		MC13892_SETTING_1_VVIDEO_2_775;
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

static int efikamx_usb_init(void)
{
	if (!of_machine_is_compatible("genesi,imx51-sb"))
		return 0;

	barebox_set_hostname("efikasb");

	mc13xxx_register_init_callback(efikamx_power_init);

	gpio_direction_output(GPIO_BLUETOOTH, 0);
	gpio_direction_output(GPIO_WIFI_ENABLE, 1);
	gpio_direction_output(GPIO_WIFI_RESET, 0);
	gpio_direction_output(GPIO_SMSC3317_RESET, 0);
	gpio_direction_output(GPIO_HUB_RESET, 0);
	gpio_direction_output(GPIO_BACKLIGHT_POWER, 1);

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

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		of_device_enable_path("/chosen/environment-sd");
		break;
	case BOOTSOURCE_SPI:
	default:
		of_device_enable_path("/chosen/environment-spi");
		break;
	}

	return 0;
}
console_initcall(efikamx_usb_init);

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

static int efikamx_late_init(void)
{
	int i;

	if (!of_machine_is_compatible("genesi,imx51-sb"))
		return 0;

	defaultenv_append_directory(defaultenv_efikasb);

	for (i = 0; i < ARRAY_SIZE(leds); i++)
		led_gpio_register(&leds[i]);

	led_set_trigger(LED_TRIGGER_HEARTBEAT, &leds[0].led);

	writew(0x0, MX51_WDOG_BASE_ADDR + 0x8);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc1",
			BBU_HANDLER_FLAG_DEFAULT);

	armlinux_set_architecture(2370);
	armlinux_set_revision(0x5100 | imx_silicon_revision());

	return 0;
}
late_initcall(efikamx_late_init);
