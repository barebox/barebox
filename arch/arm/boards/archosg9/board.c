/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-devices.h>
#include <mach/omap4_rom_usb.h>
#include <mach/omap-fb.h>
#include <linux/sizes.h>
#include <i2c/i2c.h>
#include <gpio.h>
#include <gpio_keys.h>
#include <twl6030_pwrbtn.h>
#include <readkey.h>
#include "archos_features.h"

#define GPIO_LCD_PWON     38
#define GPIO_BRIDGE_EN    39
#define GPIO_LCD_RST      53
#define GPIO_LCD_STDBY   101
#define GPIO_LCD_AVDD_EN  12
#define GPIO_BKL_EN      122
#define GPIO_BKL_LED     143

#define GPIO_5V_PWRON     36
#define GPIO_VCC_PWRON    35
#define GPIO_1V8_PWRON    34
#define GPIO_GPS_ENABLE   41

static int archosg9_console_init(void)
{
	barebox_set_model("Archos G9");
	barebox_set_hostname("g9");

	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_OMAP4_USBBOOT) &&
			omap4_usbboot_ready()) {
		add_generic_device("serial_omap4_usbboot", DEVICE_ID_DYNAMIC
			, NULL, 0, 0, 0, NULL);
	}
	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_NS16550)) {
		omap44xx_add_uart1();
	}
	return 0;
}
console_initcall(archosg9_console_init);

static int archosg9_mem_init(void){
	omap_add_ram0(SZ_1G);
	return 0;
}
mem_initcall(archosg9_mem_init);

static struct i2c_board_info i2c_devices[] = {
	{ I2C_BOARD_INFO("twl6030", 0x48), },
};

static struct twl6030_pwrbtn_platform_data pwrbtn_data = {
	.code = BB_KEY_ENTER
};
static struct gpio_keys_button keys[] = {
	{ .code = BB_KEY_UP  , .gpio = 43, .active_low = 1 },
	{ .code = BB_KEY_DOWN, .gpio = 44, .active_low = 1 },
};
static struct gpio_keys_platform_data gk_data = {
	.buttons = keys,
	.nbuttons = ARRAY_SIZE(keys),
	.fifo_size = ARRAY_SIZE(keys)*sizeof(int)
};

static struct omapfb_display const archosg9_displays[] = {
	{
		.mode = {
			.name         = "g104x1",
			.refresh      =       60,
			.xres         =     1024,
			.yres         =      768,
			.pixclock     =    96000,
			.left_margin  =      320,
			.right_margin =        1,
			.hsync_len    =      320,
			.upper_margin =       38,
			.lower_margin =       38,
			.vsync_len    =        2,
		},
		.config = (
			OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
			OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IPC |
			OMAP_DSS_LCD_DATALINES_24
		),
		.power_on_delay  =  50,
		.power_off_delay = 100,
	},
};

static void archosg9_fb_enable(int e)
{
	if (e) {
		gpio_direction_output(GPIO_LCD_PWON   , 1);
		mdelay(50);
		gpio_direction_output(GPIO_LCD_RST    , 0);
		gpio_direction_output(GPIO_LCD_AVDD_EN, 0);
		mdelay(35);
		gpio_direction_output(GPIO_BRIDGE_EN  , 1);
		mdelay(10);
		gpio_direction_output(GPIO_LCD_STDBY  , 0);
		gpio_direction_output(GPIO_BKL_EN     , 0);
	} else {
		gpio_direction_output(GPIO_BKL_EN     , 1);
		gpio_direction_output(GPIO_LCD_STDBY  , 1);
		mdelay(1);
		gpio_direction_output(GPIO_BRIDGE_EN  , 0);
		gpio_direction_output(GPIO_LCD_AVDD_EN, 1);
		mdelay(10);
		gpio_direction_output(GPIO_LCD_PWON   , 0);
		gpio_direction_output(GPIO_LCD_RST    , 1);
	}
}

static struct omapfb_platform_data archosg9_fb_data = {
	.displays     = archosg9_displays,
	.num_displays = ARRAY_SIZE(archosg9_displays),
	.dss_clk_hz   = 19200000,
	.bpp          = 32,
	.enable       = archosg9_fb_enable,
};

static int archosg9_display_init(void)
{
	omap_add_display(&archosg9_fb_data);

	gpio_direction_output(GPIO_BKL_EN     , 1);
	gpio_direction_output(GPIO_LCD_RST    , 1);
	gpio_direction_output(GPIO_LCD_PWON   , 0);
	gpio_direction_output(GPIO_BRIDGE_EN  , 0);
	gpio_direction_output(GPIO_LCD_STDBY  , 1);
	gpio_direction_output(GPIO_LCD_AVDD_EN, 1);
	gpio_direction_output(GPIO_BKL_LED    , 0);
	gpio_direction_output(GPIO_VCC_PWRON  , 1);

	return 0;
}

static int archosg9_devices_init(void){
	gpio_direction_output(GPIO_GPS_ENABLE, 0);
	gpio_direction_output(GPIO_1V8_PWRON , 1);

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	omap44xx_add_i2c1(NULL);
	omap44xx_add_mmc1(NULL);
	if (IS_ENABLED(CONFIG_KEYBOARD_TWL6030) &&
			IS_ENABLED(CONFIG_KEYBOARD_GPIO)) {
		add_generic_device_res("twl6030_pwrbtn", DEVICE_ID_DYNAMIC,
			0, 0, &pwrbtn_data);
		add_gpio_keys_device(DEVICE_ID_DYNAMIC, &gk_data);
	}

	if (IS_ENABLED(CONFIG_DRIVER_VIDEO_OMAP))
		archosg9_display_init();

	/*
	 * This should be:
	 * armlinux_set_architecture(MACH_TYPE_OMAP4_ARCHOSG9);
	 * But Archos has not registered it's board to arch/arm/tools/mach-types
	 * So here there is the hardcoded value
	 */
	armlinux_set_architecture(5032);
	armlinux_set_revision(5);
	armlinux_set_atag_appender(archos_append_atags);

	return 0;
}
device_initcall(archosg9_devices_init);
