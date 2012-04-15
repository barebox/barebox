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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <mach/silicon.h>
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
#include <mach/xload.h>
#include <i2c/i2c.h>

static struct NS16550_plat serial_plat = {
	.clock = 48000000,      /* 48MHz (APLL96/2) */
	.shift = 2,
};

static int pcm049_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, OMAP44XX_UART3_BASE, 1024,
			IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(pcm049_console_init);

static int pcm049_mem_init(void)
{
	arm_add_mem_device("ram0", 0x80000000, SZ_512M);

	add_mem_device("sram0", 0x40300000, 48 * 1024,
				   IORESOURCE_MEM_WRITEABLE);
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

static int pcm049_devices_init(void)
{
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	add_generic_device("i2c-omap", DEVICE_ID_DYNAMIC, NULL, 0x48070000, 0x1000,
				IORESOURCE_MEM, NULL);

	add_generic_device("omap-hsmmc", DEVICE_ID_DYNAMIC, NULL, 0x4809C100, SZ_4K,
			   IORESOURCE_MEM, NULL);

	gpmc_generic_init(0x10);

	pcm049_network_init();

	gpmc_generic_nand_devices_init(0, 8,
			OMAP_ECC_BCH8_CODE_HW, &omap4_nand_cfg);

#ifdef CONFIG_PARTITION
	devfs_add_partition("nand0", 0x00000, SZ_128K, PARTITION_FIXED, "xload_raw");
	dev_add_bb_dev("xload_raw", "xload");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_128K + SZ_256K, SZ_128K, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM049);

	return 0;
}
device_initcall(pcm049_devices_init);
