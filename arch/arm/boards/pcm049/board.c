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
#include <asm/io.h>
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

static struct NS16550_plat serial_plat = {
	.clock = 48000000,      /* 48MHz (APLL96/2) */
	.f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR,
	.reg_read = omap_uart_read,
	.reg_write = omap_uart_write,
};

static struct device_d pcm049_serial_device = {
	.id = -1,
	.name = "serial_ns16550",
	.map_base = OMAP44XX_UART3_BASE,
	.size = 1024,
	.platform_data = (void *)&serial_plat,
};

static int pcm049_console_init(void)
{
	/* Register the serial port */
	return register_device(&pcm049_serial_device);
}
console_initcall(pcm049_console_init);

static struct memory_platform_data sram_pdata = {
	.name = "sram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sram_dev = {
	.id = -1,
	.name = "mem",
	.map_base = 0x40300000,
	.size = 48 * 1024,
	.platform_data = &sram_pdata,
};

static struct memory_platform_data sdram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id = -1,
	.name = "mem",
	.map_base = 0x80000000,
	.size = SZ_512M,
	.platform_data = &sdram_pdata,
};

#ifdef CONFIG_MMU
static int pcm049_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x80000000, 0x80000000, 256, PMD_SECT_DEF_CACHED);
	/* warning: This shadows the second half of our ram */
	arm_create_section(0x90000000, 0x80000000, 256, PMD_SECT_DEF_UNCACHED);

	mmu_enable();

	return 0;
}
device_initcall(pcm049_mmu_init);
#endif

static struct device_d hsmmc_dev = {
	.id = -1,
	.name = "omap-hsmmc",
	.map_base = 0x4809C100,
	.size = SZ_4K,
};

static struct device_d smc911x_dev = {
	.id		= -1,
	.name		= "smc911x",
	.map_base	= 0x2C000000,
	.size		= 0x4000,
};

static struct gpmc_config net_cfg = {
	.cfg = {
		0x00001000,	/* CONF1 */
		0x001e1e01,	/* CONF2 */
		0x00080300,	/* CONF3 */
		0x1c091c09,	/* CONF4 */
		0x04181f1f,	/* CONF5 */
		0x00000FCF,	/* CONF6 */
	},
	.base = 0x2C000000,
	.size = GPMC_SIZE_16M,
};

static void pcm049_network_init(void)
{
	gpmc_cs_config(5, &net_cfg);

	register_device(&smc911x_dev);
}

static int pcm049_devices_init(void)
{
	register_device(&sdram_dev);
	register_device(&sram_dev);
	register_device(&hsmmc_dev);

	pcm049_network_init();

	gpmc_generic_nand_devices_init(0, 8, OMAP_ECC_BCH8_CODE_HW);

#ifdef CONFIG_PARTITION
	devfs_add_partition("nand0", 0x00000, SZ_128K, PARTITION_FIXED, "xload_raw");
	dev_add_bb_dev("xload_raw", "xload");
	devfs_add_partition("nand0", SZ_128K, SZ_256K, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", SZ_128K + SZ_256K, SZ_128K, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM049);

	return 0;
}
device_initcall(pcm049_devices_init);

#ifdef CONFIG_SHELL_NONE
int run_shell(void)
{
	int (*func)(void) = NULL;

	switch (omap4_bootsrc()) {
	case OMAP_BOOTSRC_MMC1:
		printf("booting from MMC1\n");
		func = omap_xload_boot_mmc();
		break;
	case OMAP_BOOTSRC_UNKNOWN:
		printf("unknown boot source. Fall back to nand\n");
	case OMAP_BOOTSRC_NAND:
		printf("booting from NAND\n");
		func = omap_xload_boot_nand(SZ_128K, SZ_256K);
		break;
	}

	if (!func) {
		printf("booting failed\n");
		while (1);
	}

	shutdown_barebox();
	func();

	while (1);
}
#endif
