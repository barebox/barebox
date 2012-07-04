/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <kernel@pengutronix.de>
 * Copyright (C) 2011 Marc Kleine-Budde, Pengutronix <mkl@pengutronix.de>
 * Copyright (C) 2011 Wolfram Sang, Pengutronix <w.sang@pengutronix.de>
 * Copyright (C) 2012 Maxime Ripard, Free Electrons <maxime.ripard@free-electrons.com>
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
 */

#include <common.h>
#include <environment.h>
#include <errno.h>
#include <fec.h>
#include <gpio.h>
#include <init.h>
#include <mci.h>
#include <io.h>

#include <mach/clock.h>
#include <mach/imx-regs.h>
#include <mach/iomux-imx28.h>
#include <mach/mci.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

#include <mach/fb.h>

#include <generated/mach-types.h>

/* setup the CPU card internal signals */
static const uint32_t cfa10036_pads[] = {
	/* duart */
	FUNC(2) | PORTF(3, 2) | VE_3_3V,
	FUNC(2) | PORTF(3, 3) | VE_3_3V,

	/* mmc0 */
	SSP0_D0 | VE_3_3V | PULLUP(1),
	SSP0_D1 | VE_3_3V | PULLUP(1),
	SSP0_D2 | VE_3_3V | PULLUP(1),
	SSP0_D3 | VE_3_3V | PULLUP(1),
	SSP0_D4 | VE_3_3V | PULLUP(1),
	SSP0_D5 | VE_3_3V | PULLUP(1),
	SSP0_D6 | VE_3_3V | PULLUP(1),
	SSP0_D7 | VE_3_3V | PULLUP(1),
	SSP0_CMD | VE_3_3V | PULLUP(1),
	SSP0_CD | VE_3_3V | PULLUP(1),
	SSP0_SCK | VE_3_3V | BITKEEPER(0),
	/* MCI slot power control 1 = off */
	PWM3_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(0),
};

static struct mxs_mci_platform_data mci_pdata = {
	.caps = MMC_MODE_8BIT,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
	.f_min = 400 * 1000,
	.f_max = 25000000,
};

static int cfa10036_mem_init(void)
{
	arm_add_mem_device("ram0", IMX_MEMORY_BASE, 128 * 1024 * 1024);

	return 0;
}
mem_initcall(cfa10036_mem_init);

static int cfa10036_devices_init(void)
{
	int i;

	/* initizalize muxing */
	for (i = 0; i < ARRAY_SIZE(cfa10036_pads); i++)
		imx_gpio_mode(cfa10036_pads[i]);

	/* enable IOCLK0 to run at the PLL frequency */
	imx_set_ioclk(0, 480000000);
	/* run the SSP unit clock at 100 MHz */
	imx_set_sspclk(0, 100000000, 1);

	armlinux_set_bootparams((void *)IMX_MEMORY_BASE + 0x100);
	armlinux_set_architecture(MACH_TYPE_CFA10036);

	add_generic_device("mxs_mci", 0, NULL, IMX_SSP0_BASE, 0,
			   IORESOURCE_MEM, &mci_pdata);

	return 0;
}
device_initcall(cfa10036_devices_init);

static int cfa10036_console_init(void)
{
	add_generic_device("stm_serial", 0, NULL, IMX_DBGUART_BASE, 8192,
			   IORESOURCE_MEM, NULL);

	return 0;
}
console_initcall(cfa10036_console_init);
