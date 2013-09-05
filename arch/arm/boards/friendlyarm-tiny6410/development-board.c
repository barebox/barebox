/*
 * Copyright (C) 2012 Juergen Beisert
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
 * The FriendlyARM's Tiny6410 evaluation board comes with all connectors and
 * devices to make the Tiny6410 CPU card work. This includes:
 *
 * - the DM9000 network controller
 * - USB/MCI connectors
 * - display connector
 *
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <gpio.h>
#include <dm9000.h>
#include <mach/devices-s3c64xx.h>
#include <mach/s3c-generic.h>
#include <mach/iomux.h>

#include "tiny6410.h"

/*
 * dm9000 network controller onboard
 * Connected to CS line 1 and interrupt line EINT7,
 * data width is 16 bit
 * Area 1: Offset 0x300...0x301
 * Area 2: Offset 0x304...0x305
 */
static struct dm9000_platform_data dm9000_data = {
	.srom = 0,	/* no serial ROM for the ethernet address */
};

static const struct s3c6410_chipselect dm900_cs = {
	.adr_setup_t = 0,
	.access_setup_t = 0,
	.access_t = 20,
	.cs_hold_t = 3,
	.adr_hold_t = 20, /* CS must be de-asserted for at least 20 ns */
	.width = 16,
};

static void tiny6410evk_setup_dm9000_cs(void)
{
	s3c6410_setup_chipselect(1, &dm900_cs);
}

static const unsigned tiny6410evk_pin_usage[] = {
	/* UART1 (V24) */
	GPA4_RXD1 | ENABLE_PU,
	GPA5_TXD1,
	GPA6_NCTS1 | ENABLE_PU,
	GPA7_NRTS1,
	/* UART2 (V24) */
	GPB0_RXD2 | ENABLE_PU,
	GPB1_TXD2,
	/* UART3 (spare, 3,3 V TTL level only) */
	GPB2_RXD3 | ENABLE_PU,
	GPB3_TXD3,
};

static int tiny6410evk_devices_init(void)
{
	int i;

	/* init CPU card specific devices first */
	tiny6410_init("FA EVK");

	/* ----------- configure the access to the outer space ---------- */
	for (i = 0; i < ARRAY_SIZE(tiny6410evk_pin_usage); i++)
		s3c_gpio_mode(tiny6410evk_pin_usage[i]);

	tiny6410evk_setup_dm9000_cs();
	add_dm9000_device(0, S3C_CS1_BASE + 0x300, S3C_CS1_BASE + 0x304,
				IORESOURCE_MEM_16BIT, &dm9000_data);
	return 0;
}
device_initcall(tiny6410evk_devices_init);

static int tiny6410evk_console_init(void)
{
	/* note: UART0 has no RTS/CTS connected */
	s3c_gpio_mode(GPA0_RXD0 | ENABLE_PU);
	s3c_gpio_mode(GPA1_TXD0);

	barebox_set_model("Friendlyarm tiny6410");
	barebox_set_hostname("tiny6410");

	s3c64xx_add_uart1();

	return 0;
}
console_initcall(tiny6410evk_console_init);
