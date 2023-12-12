// SPDX-License-Identifier: GPL-2.0-only

/**
 * @file
 * @brief Board Initialization routines for the phyCARD-A-L1
 *
 * This board is based on OMAP3530.
 * More on OMAP3530 (including documentation can be found here):
 * http://focus.ti.com/docs/prod/folders/print/omap3530.html
 *
 * This file provides initialization in two stages:
 * @li Boot time initialization - just get SDRAM working.
 * This is run from SRAM - so no case constructs and global vars can be used.
 * @li Run time initialization - this is for the rest of the initializations
 * such as flash, uart etc.
 *
 * Boot time initialization includes:
 * @li SDRAM initialization.
 * @li Pin Muxing relevant for the EVM.
 *
 * Run time initialization includes
 * @li serial @ref serial_ns16550.c driver device definition
 *
 * Originally from arch/arm/boards/omap/board-beagle.c
 *
 * Copyright (C) 2011 Phytec Messtechnik GmbH - http://www.phytec.de/
 * Juergen Kilb <j.kilb@phytec.de>
 *
 * based on code from Texas Instruments / board-beagle.c
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 * Sanjeev Premi <premi@ti.com>
 */

#include <common.h>
#include <console.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <nand.h>
#include <linux/sizes.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/err.h>
#include <mach/omap/gpmc.h>
#include <mach/omap/gpmc_nand.h>
#include <mach/omap/omap_hsmmc.h>
#include <mach/omap/sdrc.h>
#include <mach/omap/omap3-silicon.h>
#include <mach/omap/sys_info.h>
#include <mach/omap/omap3-devices.h>

#define SMC911X_BASE 0x2c000000

/**
 * @brief Initialize the serial port to be used as console.
 *
 * @return result of device registration
 */
static int pcaal1_init_console(void)
{
	barebox_set_model("Phytec phyCARD-OMAP3");
	barebox_set_hostname("phycard-omap3");

	omap3_add_uart3();

	return 0;
}
console_initcall(pcaal1_init_console);

#ifdef CONFIG_DRIVER_NET_SMC911X
/** GPMC timing for our SMSC9221 device */
static struct gpmc_config smsc_cfg = {
	.cfg = {
		0x41001000,	/*CONF1 */
		0x00040500,	/*CONF2 */
		0x00000000,	/*CONF3 */
		0x04000500,	/*CONF4 */
		0x05050505,	/*CONF5 */
		0x000002c1,	/*CONF6 */
	},
	.base = SMC911X_BASE,
	/* GPMC address map as small as possible */
	.size = GPMC_SIZE_16M,
};

/*
 * Routine: setup_net_chip
 * Description: Setting up the configuration GPMC registers specific to the
 *            Ethernet hardware.
 */
static void pcaal1_setup_net_chip(void)
{
	gpmc_cs_config(5, &smsc_cfg);
}
#endif

static int pcaal1_mem_init(void)
{

#ifdef CONFIG_OMAP_GPMC
	/*
	 * WP is made high and WAIT1 active Low
	 */
	gpmc_generic_init(0x10);
#endif
	omap3_add_sram0();


	omap_add_ram0(get_sdr_cs_size(SDRC_CS0_OSET));
	printf("found %s at SDCS0\n", size_human_readable(get_sdr_cs_size(SDRC_CS0_OSET)));

	if ((get_sdr_cs_size(SDRC_CS1_OSET) != 0) && (get_sdr_cs1_base() != OMAP_SDRC_CS0)) {
		arm_add_mem_device("ram1", get_sdr_cs1_base(), get_sdr_cs_size(SDRC_CS1_OSET));
		printf("found %s at SDCS1\n", size_human_readable(get_sdr_cs_size(SDRC_CS1_OSET)));
	}

	return 0;
}
mem_initcall(pcaal1_mem_init);

struct omap_hsmmc_platform_data pcaal1_hsmmc_plat = {
	.f_max = 26000000,
};

static struct gpmc_nand_platform_data nand_plat = {
	.device_width = 16,
	.ecc_mode = OMAP_ECC_BCH8_CODE_HW,
	.nand_cfg = &omap3_nand_cfg,
};

static int pcaal1_init_devices(void)
{
	omap_add_gpmc_nand_device(&nand_plat);

	omap3_add_mmc1(&pcaal1_hsmmc_plat);

#ifdef CONFIG_DRIVER_NET_SMC911X
	pcaal1_setup_net_chip();
	add_generic_device("smc911x", DEVICE_ID_DYNAMIC, NULL, SMC911X_BASE, SZ_4K,
			   IORESOURCE_MEM, NULL);
#endif

	armlinux_set_architecture(MACH_TYPE_PCAAL1);

	return 0;
}
device_initcall(pcaal1_init_devices);

static int pcaal1_late_init(void)
{
#ifdef CONFIG_PARTITION
	devfs_add_partition("nand0", 0x00000, SZ_128K, DEVFS_PARTITION_FIXED, "x-loader");
	dev_add_bb_dev("self_raw", "x_loader0");

	devfs_add_partition("nand0", SZ_128K, SZ_512K, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", SZ_128K + SZ_512K, SZ_128K, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif
	return 0;
}
late_initcall(pcaal1_late_init);
