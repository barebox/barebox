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
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <gpio.h>
#include <generated/mach-types.h>
#include <asm/armlinux.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>
#include <mach/iomux.h>

static const unsigned tiny6410_pin_usage[] = {
	/* UART0 */
	GPA2_GPIO | GPIO_IN | ENABLE_PU, /* CTS not connected */
	GPA3_GPIO | GPIO_IN | ENABLE_PU, /* RTS not connected */

	/* local bus' D0 ... D15 are always active */
	/* local bus' A0...A5 are always active */

	/* internal NAND memory */
	GPO0_NCS2,	/* NAND's first chip select line */
	/* NAND's second chip select line, not used */
	GPO1_GPIO | GPIO_OUT | GPIO_VAL(1),
	GPP3_FALE,
	GPP4_FCLE,
	GPP5_FWE,
	GPP6_FRE,
	GPP7_RNB, /* external pull-up */

	GPF13_GPIO  | GPIO_OUT | GPIO_VAL(0), /* OTG power supply, 0 = off */

	/* nowhere connected */
	GPO2_GPIO | GPIO_IN | ENABLE_PU,
	GPO3_GPIO | GPIO_IN | ENABLE_PU,
	GPO4_GPIO | GPIO_IN | ENABLE_PU,
	GPO5_GPIO | GPIO_IN | ENABLE_PU,

	/* local bus address lines 6...15 are nowhere connected */
	GPO6_GPIO | GPIO_IN | ENABLE_PU,
	GPO7_GPIO | GPIO_IN | ENABLE_PU,
	GPO8_GPIO | GPIO_IN | ENABLE_PU,
	GPO9_GPIO | GPIO_IN | ENABLE_PU,
	GPO10_GPIO | GPIO_IN | ENABLE_PU,
	GPO11_GPIO | GPIO_IN | ENABLE_PU,
	GPO12_GPIO | GPIO_IN | ENABLE_PU,
	GPO13_GPIO | GPIO_IN | ENABLE_PU,
	GPO14_GPIO | GPIO_IN | ENABLE_PU,
	GPO15_GPIO | GPIO_IN | ENABLE_PU,
};

static int tiny6410_mem_init(void)
{
	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s3c6410_get_memory_size());

	return 0;
}
mem_initcall(tiny6410_mem_init);

void tiny6410_init(const char *bb_name)
{
	int i;

	/* ----------- configure the access to the outer space ---------- */
	for (i = 0; i < ARRAY_SIZE(tiny6410_pin_usage); i++)
		s3c_gpio_mode(tiny6410_pin_usage[i]);

	armlinux_set_bootparams((void *)S3C_SDRAM_BASE + 0x100);
	armlinux_set_architecture(MACH_TYPE_TINY6410);
}
