/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2004
 * Mark Jonas, Freescale Semiconductor, mark.jonas@motorola.com.
 *
 * (C) Copyright 2006
 * Eric Schumann, Phytec Messtechnik GmbH
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <mach/mpc5xxx.h>
#include <mach/fec.h>
#include <types.h>
#include <partition.h>
#include <mem_malloc.h>
#include <reloc.h>

struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",
	.map_base = 0xff000000,
	.size     = 16 * 1024 * 1024,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

struct device_d sdram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0x0,
	.size     = 64 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static struct mpc5xxx_fec_platform_data fec_info = {
	.xcv_type = MII100,
};

struct device_d eth_dev = {
	.id		= -1,
	.name		= "fec_mpc5xxx",
	.map_base	= MPC5XXX_FEC,
	.platform_data	= &fec_info,
};

static int devices_init (void)
{
	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&eth_dev);

	devfs_add_partition("nor0", 0x00f00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x00f60000, 0x20000, PARTITION_FIXED, "env0");

	return 0;
}

device_initcall(devices_init);

static struct device_d psc3 = {
	.id	  = -1,
	.name     = "mpc5xxx_serial",
	.map_base = MPC5XXX_PSC3,
	.size     = 4096,
};

static struct device_d psc6 = {
	.id	  = -1,
	.name     = "mpc5xxx_serial",
	.map_base = MPC5XXX_PSC6,
	.size     = 4096,
};

static int console_init(void)
{
	register_device(&psc3);
	register_device(&psc6);
	return 0;
}

console_initcall(console_init);

void *get_early_console_base(const char *name)
{
	if (!strcmp(name, RELOC("psc3")))
		return (void *)MPC5XXX_PSC3;
	if (!strcmp(name, RELOC("psc6")))
		return (void *)MPC5XXX_PSC6;
	return NULL;
}

#include "mt46v32m16-75.h"

static void sdram_start (int hi_addr)
{
	long hi_addr_bit = hi_addr ? 0x01000000 : 0;

	/* unlock mode register */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000000 | hi_addr_bit;
	__asm__ volatile ("sync");

	/* precharge all banks */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000002 | hi_addr_bit;
	__asm__ volatile ("sync");

#if SDRAM_DDR
	/* set mode register: extended mode */
	*(vu_long *)MPC5XXX_SDRAM_MODE = SDRAM_EMODE;
	__asm__ volatile ("sync");

	/* set mode register: reset DLL */
	*(vu_long *)MPC5XXX_SDRAM_MODE = SDRAM_MODE | 0x04000000;
	__asm__ volatile ("sync");
#endif

	/* precharge all banks */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000002 | hi_addr_bit;
	__asm__ volatile ("sync");

	/* auto refresh */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000004 | hi_addr_bit;
	__asm__ volatile ("sync");

	/* set mode register */
	*(vu_long *)MPC5XXX_SDRAM_MODE = SDRAM_MODE;
	__asm__ volatile ("sync");

	/* normal operation */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | hi_addr_bit;
	__asm__ volatile ("sync");
}

/*
 * ATTENTION: Although partially referenced initdram does NOT make real use
 *            use of CFG_SDRAM_BASE. The code does not work if CFG_SDRAM_BASE
 *            is something else than 0x00000000.
 */

long int initdram (int board_type)
{
	ulong dramsize = 0;
	ulong dramsize2 = 0;

	ulong test1, test2;

	if ((ulong)RELOC(initdram) > (2 << 30)) {
		/* setup SDRAM chip selects */
		*(vu_long *)MPC5XXX_SDRAM_CS0CFG = 0x0000001b;/* 256MB at 0x0 */
		*(vu_long *)MPC5XXX_SDRAM_CS1CFG = 0x10000000;/* disabled */
		__asm__ volatile ("sync");

		/* setup config registers */
		*(vu_long *)MPC5XXX_SDRAM_CONFIG1 = SDRAM_CONFIG1;
		*(vu_long *)MPC5XXX_SDRAM_CONFIG2 = SDRAM_CONFIG2;
		__asm__ volatile ("sync");

#if SDRAM_DDR && SDRAM_TAPDELAY
		/* set tap delay */
		*(vu_long *)MPC5XXX_CDM_PORCFG = SDRAM_TAPDELAY;
		__asm__ volatile ("sync");
#endif

		/* find RAM size using SDRAM CS0 only */
		sdram_start(0);
		test1 = get_ram_size((ulong *)CFG_SDRAM_BASE, 0x10000000);
		sdram_start(1);
		test2 = get_ram_size((ulong *)CFG_SDRAM_BASE, 0x10000000);
		if (test1 > test2) {
			sdram_start(0);
			dramsize = test1;
		} else {
			dramsize = test2;
		}

		/* memory smaller than 1MB is impossible */
		if (dramsize < (1 << 20)) {
			dramsize = 0;
		}

		/* set SDRAM CS0 size according to the amount of RAM found */
		if (dramsize > 0) {
			*(vu_long *)MPC5XXX_SDRAM_CS0CFG = 0x13 + __builtin_ffs(dramsize >> 20) - 1;
		} else {
			*(vu_long *)MPC5XXX_SDRAM_CS0CFG = 0; /* disabled */
		}
	} else
		puts(RELOC("skipping sdram initialization\n"));

	/* retrieve size of memory connected to SDRAM CS0 */
	dramsize = *(vu_long *)MPC5XXX_SDRAM_CS0CFG & 0xFF;
	if (dramsize >= 0x13) {
		dramsize = (1 << (dramsize - 0x13)) << 20;
	} else {
		dramsize = 0;
	}

	/* retrieve size of memory connected to SDRAM CS1 */
	dramsize2 = *(vu_long *)MPC5XXX_SDRAM_CS1CFG & 0xFF;
	if (dramsize2 >= 0x13) {
		dramsize2 = (1 << (dramsize2 - 0x13)) << 20;
	} else {
		dramsize2 = 0;
	}

	return dramsize + dramsize2;
}

#if defined(CONFIG_OF_FLAT_TREE) && defined(CONFIG_OF_BOARD_SETUP)
void
ft_board_setup(void *blob, bd_t *bd)
{
	ft_cpu_setup(blob, bd);
}
#endif

