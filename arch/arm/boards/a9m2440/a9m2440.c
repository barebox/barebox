/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <nand.h>
#include <io.h>
#include <mach/devices-s3c24xx.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c24xx-nand.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

#include "baseboards.h"

static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(A9M2440_TACLS, A9M2440_TWRPH0, A9M2440_TWRPH1)
};

static int a9m2440_check_for_ram(uint32_t addr)
{
	uint32_t tmp1, tmp2;
	int rc = 0;

	tmp1 = readl(addr);
	tmp2 = readl(addr + sizeof(uint32_t));

	writel(0xaaaaaaaa, addr);
	writel(0x55555555, addr + sizeof(uint32_t));
	if ((readl(addr) != 0xaaaaaaaa) || (readl(addr + sizeof(uint32_t)) != 0x55555555))
		rc = 1;	/* seems no RAM */

	writel(0x55555555, addr);
	writel(0xaaaaaaaa, addr + sizeof(uint32_t));
	if ((readl(addr) != 0x55555555) || (readl(addr + sizeof(uint32_t)) != 0xaaaaaaaa))
		rc = 1;	/* seems no RAM */

	writel(tmp1, addr);
	writel(tmp2, addr + sizeof(uint32_t));

	return rc;
}

static int a9m2440_mem_init(void)
{
	/*
	 * The special SDRAM setup code for this machine will always enable
	 * both SDRAM banks. But the second SDRAM device may not exists!
	 * So we must check here, if the second bank is populated to get the
	 * correct RAM size.
	 */
	switch (readl(S3C_BANKSIZE) & 0x7) {
	case 0:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 32 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	case 1:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 64 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	case 2:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 128 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	case 4:
	case 5:
	case 6:		/* not supported on this machine */
		break;
	default:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 16 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	}

	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s3c24xx_get_memory_size());

	return 0;
}
mem_initcall(a9m2440_mem_init);

static int a9m2440_devices_init(void)
{
	uint32_t reg;

	/* ----------- configure the access to the outer space ---------- */
	reg = readl(S3C_BWSCON);

	/* CS#5 to access the network controller */
	reg &= ~0x00f00000;
	reg |=  0x00d00000;	/* 16 bit */
	writel(0x1f4c, S3C_BANKCON5);

	writel(reg, S3C_BWSCON);

#ifdef CONFIG_MACH_A9M2410DEV
	a9m2410dev_devices_init();
#endif

	/* release the reset signal to external devices */
	reg = readl(S3C_MISCCR);
	reg |= 0x10000;
	writel(reg, S3C_MISCCR);

	/* ----------- the devices the boot loader should work with -------- */
	s3c24xx_add_nand(&nand_info);
	/*
	 * cs8900 network controller onboard
	 * Connected to CS line 5 + A24 and interrupt line EINT9,
	 * data width is 16 bit
	 */
	add_generic_device("cs8900", DEVICE_ID_DYNAMIC, NULL,
			S3C_CS5_BASE + (1 << 24) + 0x300, 16, IORESOURCE_MEM, NULL);

#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif
	armlinux_set_architecture(MACH_TYPE_A9M2440);

	return 0;
}

device_initcall(a9m2440_devices_init);

static int a9m2440_console_init(void)
{
	barebox_set_model("Digi A9M2440");
	barebox_set_hostname("a9m2440");

	s3c24xx_add_uart1();
	return 0;
}

console_initcall(a9m2440_console_init);
