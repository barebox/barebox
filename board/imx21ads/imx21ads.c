/*
 * Copyright (C) 2009 Ivo Clarysse
 * 
 * Based on imx27ads.c,
 *   Copyright (C) 2007 Sascha Hauer, Pengutronix 
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
 *
 */

#include <common.h>
#include <net.h>
#include <cfi_flash.h>
#include <init.h>
#include <environment.h>
#include <mach/imx-regs.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/mach-types.h>
#include <mach/imx-nand.h>

static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.map_base = 0xC8000000,
	.size     = 32 * 1024 * 1024,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.name     = "mem",
	.map_base = 0xc0000000,
	.size     = 64 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

struct imx_nand_platform_data nand_info = {
	.width = 1,
	.hw_ecc = 1,
};

static struct device_d nand_dev = {
	.name     = "imx_nand",
	.map_base = 0xDF003000,
	.platform_data  = &nand_info,
};

static struct device_d cs8900_dev = {
	.name     = "cs8900",
	.map_base = IMX_CS1_BASE,
	// IRQ is connected to UART3_RTS
};

static int imx21ads_timing_init(void)
{
	u32 temp;

	/* Configure External Interface Module */
	/* CS0: burst flash */
	CS0U = 0x00003E00;
	CS0L = 0x00000E01;

	/* CS1: Ethernet controller, external UART, memory-mapped I/O (16-bit) */
	CS1U = 0x00002000;
	CS1L = 0x11118501;

	/* CS2: disable (not available, since CSD0 in use) */
	CS2U = 0x0;
	CS2L = 0x0;

	/* CS3: disable */
	CS3U = 0x0;
	CS3L = 0x0;
	/* CS4: disable */
	CS4U = 0x0;
	CS4L = 0x0;
	/* CS5: disable */
	CS5U = 0x0;
	CS5L = 0x0;

	temp = PCDR0;
	temp &= ~0xF000;
	temp |= 0xA000;  /* Set NFC divider; 0xA yields 24.18MHz */
	PCDR0 = temp;

	return 0;
}

core_initcall(imx21ads_timing_init);

static int mx21ads_devices_init(void)
{
	int i;
	unsigned int mode[] = {
		PE12_PF_UART1_TXD,
		PE13_PF_UART1_RXD,
		PE14_PF_UART1_CTS,
		PE15_PF_UART1_RTS,
	};

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(mode); i++)
		imx_gpio_mode(mode[i]);

	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&nand_dev);
	register_device(&cs8900_dev);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0xc0000100);
	armlinux_set_architecture(MACH_TYPE_MX21ADS);

	return 0;
}

device_initcall(mx21ads_devices_init);

static struct device_d mx21ads_serial_device = {
	.name     = "imx_serial",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
};

static int mx21ads_console_init(void)
{
	register_device(&mx21ads_serial_device);
	return 0;
}

console_initcall(mx21ads_console_init);
