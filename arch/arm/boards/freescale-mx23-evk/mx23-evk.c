/*
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
#include <mci.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <mach/mci.h>

static struct mxs_mci_platform_data mci_pdata = {
	.caps = MMC_MODE_4BIT,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
};

static const uint32_t pad_setup[] = {
	/* SD card interface */
	SSP1_DATA0 | PULLUP(1),
	SSP1_DATA1 | PULLUP(1),
	SSP1_DATA2 | PULLUP(1),
	SSP1_DATA3 | PULLUP(1),
	SSP1_SCK,
	SSP1_CMD | PULLUP(1),
	SSP1_DETECT | PULLUP(1),
};

static int mx23_evk_mem_init(void)
{
	arm_add_mem_device("ram0", IMX_MEMORY_BASE, 32 * 1024 * 1024);

	return 0;
}
mem_initcall(mx23_evk_mem_init);

static int mx23_evk_devices_init(void)
{
	int i;

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(pad_setup); i++)
		imx_gpio_mode(pad_setup[i]);

	armlinux_set_bootparams((void*)IMX_MEMORY_BASE + 0x100);
	armlinux_set_architecture(MACH_TYPE_MX23EVK);

	imx_set_ioclk(480000000); /* enable IOCLK to run at the PLL frequency */
	imx_set_sspclk(0, 100000000, 1);
	add_generic_device("mxs_mci", 0, NULL, IMX_SSP1_BASE, 0,
			   IORESOURCE_MEM, &mci_pdata);

	return 0;
}

device_initcall(mx23_evk_devices_init);

static int mx23_evk_console_init(void)
{
	add_generic_device("stm_serial", 0, NULL, IMX_DBGUART_BASE, 8192,
			   IORESOURCE_MEM, NULL);
	
	return 0;
}

console_initcall(mx23_evk_console_init);

/** @page mx23_evk Freescale's i.MX23 evaluation kit

This CPU card is based on an i.MX23 CPU. The card is shipped with:

- 32 MiB synchronous dynamic RAM (mobile DDR type)
- ENC28j60 based network (over SPI)

Memory layout when @b barebox is running:

- 0x40000000 start of SDRAM
- 0x40000100 start of kernel's boot parameters
  - below malloc area: stack area
  - below barebox: malloc area
- 0x41000000 start of @b barebox

@section get_imx23evk_binary How to get the bootloader binary image:

Using the default configuration:

@verbatim
make ARCH=arm imx23evk_defconfig
@endverbatim

Build the bootloader binary image:

@verbatim
make ARCH=arm CROSS_COMPILE=armv5compiler
@endverbatim

@note replace the armv5compiler with your ARM v5 cross compiler.
*/
