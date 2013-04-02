/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
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
#include <console.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-devices.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/control.h>
#include <linux/err.h>
#include <sizes.h>
#include <partition.h>
#include <nand.h>
#include <asm/mmu.h>
#include <mach/gpio.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>
#include <i2c/i2c.h>

static int pcm049_console_init(void)
{
	omap44xx_add_uart3();

	return 0;
}
console_initcall(pcm049_console_init);

static int pcm049_mem_init(void)
{
	omap_add_ram0(SZ_512M);

	omap44xx_add_sram0();
	return 0;
}
mem_initcall(pcm049_mem_init);

static struct gpmc_config net_cfg = {
	.cfg = {
		0xc1001000,	/* CONF1 */
		0x00070700,	/* CONF2 */
		0x00000000,	/* CONF3 */
		0x07000700,	/* CONF4 */
		0x09060909,	/* CONF5 */
		0x000003c2,	/* CONF6 */
	},
	.base = 0x2C000000,
	.size = GPMC_SIZE_16M,
};

static void pcm049_network_init(void)
{
	gpmc_cs_config(5, &net_cfg);

	add_generic_device("smc911x", DEVICE_ID_DYNAMIC, NULL, 0x2C000000, 0x4000,
			   IORESOURCE_MEM, NULL);
}

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("twl6030", 0x48),
	},
};

static struct gpmc_nand_platform_data nand_plat = {
	.wait_mon_pin = 1,
	.ecc_mode = OMAP_ECC_BCH8_CODE_HW,
	.nand_cfg = &omap4_nand_cfg,
};

static int pcm049_devices_init(void)
{
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	omap44xx_add_i2c1(NULL);
	omap44xx_add_mmc1(NULL);

	gpmc_generic_init(0x10);

	pcm049_network_init();

	omap_add_gpmc_nand_device(&nand_plat);

#ifdef CONFIG_PARTITION
	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "xload_raw");
	dev_add_bb_dev("xload_raw", "xload");
	devfs_add_partition("nand0", SZ_128K, SZ_512K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_128K + SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM049);

	return 0;
}
device_initcall(pcm049_devices_init);
