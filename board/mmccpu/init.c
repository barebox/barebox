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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
#include <fec.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <asm/arch/ether.h>

static struct device_d sdram_dev = {
	.name		= "ram",
	.id		= "ram0",

	.map_base	= 0x20000000,
	.size		= 128 * 1024 * 1024,

	.type		= DEVICE_TYPE_DRAM,
};

static struct device_d cfi_dev = {
	.name		= "cfi_flash",
	.id		= "nor0",

	.map_base	= 0x10000000,
	.size		= 0,	/* zero means autodetect size */
};

static struct at91sam_ether_platform_data macb_pdata = {
	.flags		= AT91SAM_ETHER_MII | AT91SAM_ETHER_FORCE_LINK,
	.phy_addr	= 4,
};

static struct device_d macb_dev = {
	.name		= "macb",
	.id		= "eth0",
	.map_base	= AT91C_BASE_MACB,
	.size		= 0x1000,
	.platform_data	= &macb_pdata,
};

static int mmccpu_devices_init(void)
{
	u32 pe = AT91C_PC25_ERXDV |
		AT91C_PC22_ERX2 |
		AT91C_PC23_ERX3 |
		AT91C_PC20_ETX2 |
		AT91C_PC21_ETX3;

	writel(pe, AT91C_BASE_PIOC + PIO_BSR(0));
	writel(pe, AT91C_BASE_PIOC + PIO_PDR(0));

	pe = AT91C_PE21_ETXCK |
		AT91C_PE23_ETX0 |
		AT91C_PE24_ETX1 |
		AT91C_PE25_ERX0 |
		AT91C_PE26_ERX1 |
		AT91C_PE27_ERXER |
		AT91C_PE28_ETXEN |
		AT91C_PE29_EMDC |
		AT91C_PE30_EMDIO;

	writel(pe, AT91C_BASE_PIOE + PIO_ASR(0));
	writel(pe, AT91C_BASE_PIOE + PIO_PDR(0));

	/* set PB27 to '1', enable 50MHz oscillator */
	writel(AT91C_PIO_PB27, AT91C_BASE_PIOB + PIO_PER(0));
	writel(AT91C_PIO_PB27, AT91C_BASE_PIOB + PIO_OER(0));
	writel(AT91C_PIO_PB27, AT91C_BASE_PIOB + PIO_SODR(0));

	/* set PB4, PB5 to '1', enable 50MHz oscillator */
	writel(AT91C_PIO_PB4|AT91C_PIO_PB5, AT91C_BASE_PIOB + PIO_PER(0));
	writel(AT91C_PIO_PB4|AT91C_PIO_PB5, AT91C_BASE_PIOB + PIO_OER(0));
	writel(AT91C_PIO_PB4|AT91C_PIO_PB5, AT91C_BASE_PIOB + PIO_SODR(0));

	writel(1 << AT91C_ID_EMAC, AT91C_PMC_PCER);

	register_device(&sdram_dev);
	register_device(&macb_dev);
	register_device(&cfi_dev);

	devfs_add_partition("nor0", 0x00000, 256 * 1024, PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x40000, 128 * 1024, PARTITION_FIXED, "env");

	armlinux_set_bootparams((void *)0x20000100);
	armlinux_set_architecture(MACH_TYPE_MMCCPU);

	return 0;
}

device_initcall(mmccpu_devices_init);

static struct device_d mmccpu_serial_device = {
	.name		= "atmel_serial",
	.id		= "cs0",
	.map_base	= AT91C_BASE_DBGU,
	.size		= 4096,
	.type		= DEVICE_TYPE_CONSOLE,
};

static int mmccpu_console_init(void)
{
	writel(AT91C_PC31_DTXD | AT91C_PC30_DRXD, AT91C_PIOC_PDR);

	register_device(&mmccpu_serial_device);
	return 0;
}

console_initcall(mmccpu_console_init);
