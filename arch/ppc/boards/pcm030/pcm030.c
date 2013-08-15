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
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <fec.h>
#include <types.h>
#include <partition.h>
#include <memory.h>
#include <sizes.h>
#include <linux/stat.h>
#include <fs.h>

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_MII,
};

static int devices_init (void)
{
	struct stat s;
	int ret;

	/*
	 * Flash can be 16MB or 32MB, setup for the last 32MB no matter
	 * what we find later.
	 */
	mpc5200_setup_cs(MPC5200_BOOTCS, 0xfe000000, SZ_32M, 0x0008fd00);
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xfe000000, 32 * 1024 * 1024, 0);

	add_generic_device("fec_mpc5xxx", DEVICE_ID_DYNAMIC, NULL, MPC5XXX_FEC, 0x200,
			   IORESOURCE_MEM, &fec_info);

	ret = stat("/dev/nor0", &s);
	if (ret)
		return 0;

	devfs_add_partition("nor0", s.st_size - SZ_1M, SZ_512K, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", s.st_size - SZ_512K, SZ_512K, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}

device_initcall(devices_init);

static int console_init(void)
{
	barebox_set_model("Phytec phyCORE MPC5200 tiny");
	barebox_set_hostname("mpc5200");

	add_generic_device("mpc5xxx_serial", DEVICE_ID_DYNAMIC, NULL, MPC5XXX_PSC3, 0x200,
			   IORESOURCE_MEM, NULL);
	add_generic_device("mpc5xxx_serial", DEVICE_ID_DYNAMIC, NULL, MPC5XXX_PSC6, 0x200,
			   IORESOURCE_MEM, NULL);
	return 0;
}

console_initcall(console_init);

static int mem_init(void)
{
	unsigned long sdramsize;

	sdramsize = mpc5200_get_sdram_size(0) + mpc5200_get_sdram_size(1);

	barebox_add_memory_bank("ram0", 0x0, sdramsize);

	return 0;
}
mem_initcall(mem_init);

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

void initdram (int board_type)
{
	ulong dramsize = 0;

	ulong test1, test2;

	/* Setup pin multiplexing */

	/* PSC6=UART, PSC3=UART ; Ether=100MBit with MD */
	*(vu_long *)MPC5XXX_GPS_PORT_CONFIG = 0x00558c10;
	*(vu_long *)MPC5XXX_CS_BURST = 0x00000000;
	*(vu_long *)MPC5XXX_CS_DEADCYCLE = 0x33333333;

	mpc5200_setup_bus_clocks(1, 4);

	if (get_pc() > SZ_128M) {
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
		test1 = get_ram_size((ulong *)0, 0x10000000);
		sdram_start(1);
		test2 = get_ram_size((ulong *)0, 0x10000000);
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
	}
}
