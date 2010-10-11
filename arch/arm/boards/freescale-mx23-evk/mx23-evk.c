/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/imx-regs.h>

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.name     = "mem",
	.map_base = IMX_MEMORY_BASE,
	.size     = 32 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static int mx23_evk_devices_init(void)
{
	register_device(&sdram_dev);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void*)(sdram_dev.map_base + 0x100));
	armlinux_set_architecture(MACH_TYPE_MX23EVK);

	return 0;
}

device_initcall(mx23_evk_devices_init);

static struct device_d mx23_evk_serial_device = {
	.name     = "stm_serial",
	.map_base = IMX_DBGUART_BASE,
	.size     = 8192,
};

static int mx23_evk_console_init(void)
{
	return register_device(&mx23_evk_serial_device);
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
