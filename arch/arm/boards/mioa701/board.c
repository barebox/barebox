/*
 * (C) 2011 Robert Jarzmik <robert.jarzmik@free.fr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <init.h>
#include <partition.h>
#include <led.h>
#include <gpio.h>
#include <pwm.h>

#include <mach/devices.h>
#include <mach/mfp-pxa27x.h>
#include <mach/pxa-regs.h>
#include <mach/udc_pxa2xx.h>
#include <mach/mci_pxa2xx.h>

#include <asm/armlinux.h>
#include <asm/io.h>
#include <generated/mach-types.h>
#include <asm/mmu.h>

#include "mioa701.h"

/*
 * LTM0305A776C LCD panel timings
 *
 * see:
 *  - the LTM0305A776C datasheet,
 *  - and the PXA27x Programmers' manual
 */
static struct pxafb_videomode mioa701_ltm0305a776c = {
	{
		.pixclock		= 220000,	/* CLK=4.545 MHz */
		.xres			= 240,
		.yres			= 320,
		.hsync_len		= 4,
		.vsync_len		= 2,
		.left_margin		= 6,
		.right_margin		= 4,
		.upper_margin		= 5,
		.lower_margin		= 3,
	},
	.bpp = 16,
};

static void mioa701_lcd_power(int on)
{
	gpio_set_value(GPIO87_LCD_POWER, on);
}

static void mioa701_lcd_backlight(int on)
{
	struct pwm_device *pwm0 = pwm_request("pxa_pwm0");

	/*
	 * The backlight has a base frequency of 250kHz (<=> 4 ms).
	 */
	if (on) {
		pwm_enable(pwm0);
		pwm_config(pwm0, 2000 * 1024, 4000 * 1024);
	} else {
		pwm_disable(pwm0);
	}
	pwm_free(pwm0);
}

static struct pxafb_platform_data mioa701_pxafb_info = {
	.mode			= &mioa701_ltm0305a776c,
	.lcd_conn		= LCD_COLOR_TFT_16BPP | LCD_PCLK_EDGE_FALL,
	.lcd_power		= mioa701_lcd_power,
	.backlight_power	= mioa701_lcd_backlight,
};

#define MIO_LED(_name, _gpio) \
	{ .gpio = _gpio, .active_low = 1, .led = { .name = #_name, } }
static struct gpio_led leds[] = {
	MIO_LED(charging, GPIO10_LED_nCharging),
	MIO_LED(blue, GPIO97_LED_nBlue),
	MIO_LED(orange, GPIO98_LED_nOrange),
	MIO_LED(vibra, GPIO82_LED_nVibra),
	MIO_LED(keyboard, GPIO115_LED_nKeyboard),
};


static int is_usb_connected(void)
{
	return !gpio_get_value(GPIO13_nUSB_DETECT);
}

static struct pxa2xx_udc_mach_info mioa701_udc_info = {
	.udc_is_connected = is_usb_connected,
	.gpio_pullup	  = GPIO22_USB_ENABLE,
};

static struct pxamci_platform_data mioa701_mmc_info = {
	.gpio_power = GPIO91_SDIO_EN,
};

static int mioa701_devices_init(void)
{
	int i;
	void *docg3_iospace;

	pxa_add_pwm((void *)0x40b00000, 0);
	pxa_add_fb((void *)0x44000000, &mioa701_pxafb_info);
	pxa_add_mmc((void *)0x41100000, DEVICE_ID_DYNAMIC, &mioa701_mmc_info);
	docg3_iospace = map_io_sections(0x0, (void *)0xe0000000, 0x2000);
	add_generic_device("docg3", DEVICE_ID_DYNAMIC, NULL, (ulong) docg3_iospace,
			0x2000, IORESOURCE_MEM, NULL);
	armlinux_set_bootparams((void *)0xa0000100);
	armlinux_set_architecture(MACH_TYPE_MIOA701);

	for (i = 0; i < ARRAY_SIZE(leds); i++)
		led_gpio_register(&leds[i]);
	add_generic_device("pxa27x-udc", 0, NULL, 0x40600000,
			   1024, IORESOURCE_MEM, &mioa701_udc_info);
	return 0;
}

device_initcall(mioa701_devices_init);

static unsigned long mioa701_pin_config[] = {
	/* Mio global */
	MIO_CFG_OUT(GPIO9_CHARGE_EN, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO18_POWEROFF, AF0, DRIVE_LOW),
	MFP_CFG_OUT(GPIO3, AF0, DRIVE_HIGH),
	MFP_CFG_OUT(GPIO4, AF0, DRIVE_HIGH),
	MIO_CFG_IN(GPIO80_MAYBE_CHARGE_VDROP, AF0),

	/* Backlight PWM 0 */
	GPIO16_PWM0_OUT,

	/* LCD */
	GPIOxx_LCD_TFT_16BPP,
	MIO_CFG_OUT(GPIO87_LCD_POWER, AF0, DRIVE_LOW),

	/* MMC */
	GPIO32_MMC_CLK,
	GPIO92_MMC_DAT_0,
	GPIO109_MMC_DAT_1,
	GPIO110_MMC_DAT_2,
	GPIO111_MMC_DAT_3,
	GPIO112_MMC_CMD,
	MIO_CFG_IN(GPIO78_SDIO_RO, AF0),
	MIO_CFG_IN(GPIO15_SDIO_INSERT, AF0),
	MIO_CFG_OUT(GPIO91_SDIO_EN, AF0, DRIVE_LOW),

	/* USB */
	MIO_CFG_IN(GPIO13_nUSB_DETECT, AF0),
	MIO_CFG_OUT(GPIO22_USB_ENABLE, AF0, DRIVE_LOW),

	/* QCI */
	GPIO12_CIF_DD_7,
	GPIO17_CIF_DD_6,
	GPIO50_CIF_DD_3,
	GPIO51_CIF_DD_2,
	GPIO52_CIF_DD_4,
	GPIO53_CIF_MCLK,
	GPIO54_CIF_PCLK,
	GPIO55_CIF_DD_1,
	GPIO81_CIF_DD_0,
	GPIO82_CIF_DD_5,
	GPIO84_CIF_FV,
	GPIO85_CIF_LV,

	/* Bluetooth */
	MIO_CFG_IN(GPIO14_BT_nACTIVITY, AF0),
	GPIO44_BTUART_CTS,
	GPIO42_BTUART_RXD,
	GPIO45_BTUART_RTS,
	GPIO43_BTUART_TXD,
	MIO_CFG_OUT(GPIO83_BT_ON, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO77_BT_UNKNOWN1, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO86_BT_MAYBE_nRESET, AF0, DRIVE_HIGH),

	/* GPS */
	MIO_CFG_OUT(GPIO23_GPS_UNKNOWN1, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO26_GPS_ON, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO27_GPS_RESET, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO106_GPS_UNKNOWN2, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO107_GPS_UNKNOWN3, AF0, DRIVE_LOW),
	GPIO46_STUART_RXD,
	GPIO47_STUART_TXD,

	/* GSM */
	MIO_CFG_OUT(GPIO24_GSM_MOD_RESET_CMD, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO88_GSM_nMOD_ON_CMD, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO90_GSM_nMOD_OFF_CMD, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO114_GSM_nMOD_DTE_UART_STATE, AF0, DRIVE_HIGH),
	MIO_CFG_IN(GPIO25_GSM_MOD_ON_STATE, AF0),
	MIO_CFG_IN(GPIO113_GSM_EVENT, AF0) | WAKEUP_ON_EDGE_BOTH,
	GPIO34_FFUART_RXD,
	GPIO35_FFUART_CTS,
	GPIO36_FFUART_DCD,
	GPIO37_FFUART_DSR,
	GPIO39_FFUART_TXD,
	GPIO40_FFUART_DTR,
	GPIO41_FFUART_RTS,

	/* Sound */
	GPIO28_AC97_BITCLK,
	GPIO29_AC97_SDATA_IN_0,
	GPIO30_AC97_SDATA_OUT,
	GPIO31_AC97_SYNC,
	GPIO89_AC97_SYSCLK,
	MIO_CFG_IN(GPIO12_HPJACK_INSERT, AF0),

	/* Leds */
	MIO_CFG_OUT(GPIO10_LED_nCharging, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO97_LED_nBlue, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO98_LED_nOrange, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO82_LED_nVibra, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO115_LED_nKeyboard, AF0, DRIVE_HIGH),

	/* Keyboard */
	MIO_CFG_IN(GPIO0_KEY_POWER, AF0) | WAKEUP_ON_EDGE_BOTH,
	MIO_CFG_IN(GPIO93_KEY_VOLUME_UP, AF0),
	MIO_CFG_IN(GPIO94_KEY_VOLUME_DOWN, AF0),
	GPIO100_KP_MKIN_0,
	GPIO101_KP_MKIN_1,
	GPIO102_KP_MKIN_2,
	GPIO103_KP_MKOUT_0,
	GPIO104_KP_MKOUT_1,
	GPIO105_KP_MKOUT_2,

	/* I2C */
	GPIO117_I2C_SCL,
	GPIO118_I2C_SDA,

	/* Unknown */
	MFP_CFG_IN(GPIO20, AF0),
	MFP_CFG_IN(GPIO21, AF0),
	MFP_CFG_IN(GPIO33, AF0),
	MFP_CFG_OUT(GPIO49, AF0, DRIVE_HIGH),
	MFP_CFG_OUT(GPIO57, AF0, DRIVE_HIGH),
	MFP_CFG_IN(GPIO96, AF0),
	MFP_CFG_OUT(GPIO116, AF0, DRIVE_HIGH),
};

static int mioa701_coredevice_init(void)
{
	unsigned int cclk;
	/* route pins */
	pxa2xx_mfp_config(ARRAY_AND_SIZE(mioa701_pin_config));

	/*
	 * Put the board in superspeed (520 MHz) to speed-up logo/OS loading.
	 * This requires to command the Maxim 1586 to upgrade core voltage to
	 * 1.475 V, on the power I2C bus (device 0x14).
	 */
	CCCR = CCCR_A | 0x20290;
	PCFR = PCFR_GPR_EN | PCFR_FVC | PCFR_DC_EN | PCFR_PI2C_EN | PCFR_OPDE;
	PCMD(0) = PCMD_LC | 0x1f;
	PVCR = 0x14;

	cclk = 0x0b;
	asm volatile("mcr p14, 0, %0, c6, c0, 0 @ set CCLK"
	  : : "r" (cclk) : "cc");

	barebox_set_model("Scoter Mitac Mio A701");
	barebox_set_hostname("mioa701");

	return 0;
}
coredevice_initcall(mioa701_coredevice_init);

static int mioa701_mem_init(void)
{
	arm_add_mem_device("ram0", 0xa0000000, 64 * 1024 * 1024);
	return 0;
}
mem_initcall(mioa701_mem_init);
