// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 Raghavendra KH <r-khandenahally@ti.com>, Texas Instruments (http://www.ti.com/)
// SPDX-FileCopyrightText: 2012 Jan Luebbe <j.luebbe@pengutronix.de>

/**
 * @file
 * @brief BeagleBone Specific Board Initialization routines
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <envfs.h>
#include <environment.h>
#include <globalvar.h>
#include <linux/sizes.h>
#include <net.h>
#include <bootsource.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <mach/omap/am33xx-silicon.h>
#include <mach/omap/generic.h>
#include <mach/omap/sys_info.h>
#include <mach/omap/syslib.h>
#include <mach/omap/gpmc.h>
#include <linux/err.h>
#include <mach/omap/bbu.h>

#include "beaglebone.h"

static int beaglebone_coredevice_init(void)
{
	if (!of_machine_is_compatible("ti,am335x-bone"))
		return 0;

	am33xx_register_ethaddr(0, 0);
	return 0;
}
coredevice_initcall(beaglebone_coredevice_init);

static int beaglebone_mem_init(void)
{
	uint32_t sdram_size;

	if (!of_machine_is_compatible("ti,am335x-bone"))
		return 0;

	if (is_beaglebone_black())
		sdram_size = SZ_512M;
	else
		sdram_size = SZ_256M;

	arm_add_mem_device("ram0", OMAP_DRAM_ADDR_SPACE_START, sdram_size);

	return 0;
}
mem_initcall(beaglebone_mem_init);

static int beaglebone_devices_init(void)
{
	int black;

	if (!of_machine_is_compatible("ti,am335x-bone"))
		return 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 0)
			omap_set_bootmmc_devname("mmc0");
		else
			omap_set_bootmmc_devname("mmc1");
	}

	black = is_beaglebone_black();

	defaultenv_append_directory(defaultenv_beaglebone);

	globalvar_add_simple("board.variant", black ? "boneblack" : "bone");

	printf("detected 'BeagleBone %s'\n", black ? "Black" : "White");

	armlinux_set_architecture(MACH_TYPE_BEAGLEBONE);

	/* Register update handlers */
	am33xx_bbu_emmc_mlo_register_handler("MLO.emmc", "/dev/mmc1");

	bbu_register_std_file_update("MLO.fat.emmc", 0, "/mnt/mmc1.0/MLO",
				     filetype_ch_image);
	bbu_register_std_file_update("barebox.fat.emmc", 0, "/mnt/mmc1.0/barebox.bin",
				     filetype_arm_barebox);

	bbu_register_std_file_update("MLO.fat.sd", 0, "/mnt/mmc0.0/MLO",
				     filetype_ch_image);
	bbu_register_std_file_update("barebox.fat.sd", 0, "/mnt/mmc0.0/barebox.bin",
				     filetype_arm_barebox);

	if (IS_ENABLED(CONFIG_SHELL_NONE))
		return am33xx_of_register_bootdevice();

	return 0;
}
coredevice_initcall(beaglebone_devices_init);
