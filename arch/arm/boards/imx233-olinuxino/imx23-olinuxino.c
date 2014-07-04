/*
 * (C) Copyright 2012 Fadil Berisha, <fadil.r.berisha@gmail.com>
 *     based on falconwing.c & mx23-evk.c
 *
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 * (C) Copyright 2011 Wolfram Sang - Pengutronix
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
#include <init.h>
#include <gpio.h>
#include <environment.h>
#include <envfs.h>
#include <errno.h>
#include <mci.h>
#include <asm/armlinux.h>
#include <usb/ehci.h>
#include <mach/usb.h>
#include <generated/mach-types.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <mach/mci.h>
#include <mach/iomux.h>

static struct mxs_mci_platform_data mci_pdata = {
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
	.f_min = 400000,
};

static const uint32_t pad_setup[] = {
	/* debug port */
	PWM1_DUART_TX | STRENGTH(S4MA),    /* PWM0/DUART_TXD - U_DEBUG PIN 2 */
	PWM0_DUART_RX | STRENGTH(S4MA),    /* PWM0/DUART_RXD - U_DEBUG PIN 1 */

	/* auart */
	I2C_SDA_AUART1_RX | STRENGTH(S4MA),
	I2C_CLK_AUART1_TX | STRENGTH(S4MA),

	/* lcd */
	LCD_D17 | STRENGTH(S12MA),         /*PIN18/LCD_D17   -   GPIO PIN 3 */
	LCD_D16 | STRENGTH(S12MA),
	LCD_D15 | STRENGTH(S12MA),
	LCD_D14 | STRENGTH(S12MA),
	LCD_D13 | STRENGTH(S12MA),
	LCD_D12 | STRENGTH(S12MA),
	LCD_D11 | STRENGTH(S12MA),
	LCD_D10 | STRENGTH(S12MA),
	LCD_D9 | STRENGTH(S12MA),
	LCD_D8 | STRENGTH(S12MA),
	LCD_D7 | STRENGTH(S12MA),
	LCD_D6 | STRENGTH(S12MA),
	LCD_D5 | STRENGTH(S12MA),
	LCD_D4 | STRENGTH(S12MA),
	LCD_D3 | STRENGTH(S12MA),
	LCD_D2 | STRENGTH(S12MA),            /* PIN3/LCD_D02   - GPIO PIN 31*/
	LCD_D1 | STRENGTH(S12MA),            /* PIN2/LCD_D01   - GPIO PIN 33*/
	LCD_D0 | STRENGTH(S12MA),            /* PIN1/LCD_D00   - GPIO PIN 35*/
	LCD_CS,                              /* PIN26/LCD_CS   - GPIO PIN 20*/
	LCD_RS,                              /* PIN25/LCD_RS   - GPIO PIN 18*/
	LCD_WR,                              /* PIN24/LCD_WR   - GPIO PIN 16*/
	LCD_RESET,                           /* PIN23/LCD_DISP - GPIO PIN 14*/
	LCD_ENABE | STRENGTH(S12MA),  /* PIN22/LCD_EN/I2C_SCL  - GPIO PIN 12*/
	LCD_VSYNC | STRENGTH(S12MA), /* PIN21/LCD_HSYNC/I2C_SDA- GPIO PIN 10*/
	LCD_HSYNC | STRENGTH(S12MA),     /* PIN20/LCD_VSYNC    - GPIO PIN  8*/
	LCD_DOTCLOCK | STRENGTH(S12MA),  /* PIN19/LCD_DOTCLK   - GPIO PIN  6*/


	/* SD card interface */
	SSP1_DATA0 | PULLUP(1),
	SSP1_DATA1 | PULLUP(1),
	SSP1_DATA2 | PULLUP(1),
	SSP1_DATA3 | PULLUP(1),
	SSP1_SCK,
	SSP1_CMD | PULLUP(1),
	SSP1_DETECT | PULLUP(1),

	/* led */
	SSP1_DETECT_GPIO | GPIO_OUT | GPIO_VALUE(1),

	/* gpio - USB hub LAN9512-JZX*/
	GPMI_ALE_GPIO | GPIO_OUT | GPIO_VALUE(1),
};

static int imx23_olinuxino_mem_init(void)
{
	arm_add_mem_device("ram0", IMX_MEMORY_BASE, 64 * 1024 * 1024);

	return 0;
}
mem_initcall(imx23_olinuxino_mem_init);

static void olinuxino_init_usb(void)
{

	imx23_usb_phy_enable();

	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, IMX_USB_BASE, NULL);
}

static int imx23_olinuxino_devices_init(void)
{
	int i;

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(pad_setup); i++)
		imx_gpio_mode(pad_setup[i]);

	armlinux_set_architecture(MACH_TYPE_IMX233_OLINUXINO);

	add_generic_device("mxs_mci", DEVICE_ID_DYNAMIC, NULL, IMX_SSP1_BASE,
					0x8000, IORESOURCE_MEM, &mci_pdata);

	olinuxino_init_usb();

	default_environment_path_set("/dev/disk0.1");

	return 0;
}

device_initcall(imx23_olinuxino_devices_init);

static int imx23_olinuxino_console_init(void)
{
	barebox_set_model("Olimex.ltd imx233-olinuxino");
	barebox_set_hostname("imx233-olinuxino");

	add_generic_device("stm_serial", 0, NULL, IMX_DBGUART_BASE, 8192,
			   IORESOURCE_MEM, NULL);

	return 0;
}

console_initcall(imx23_olinuxino_console_init);
