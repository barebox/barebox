/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Raghavendra KH <r-khandenahally@ti.com>
 *
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
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
 */

/**
 * @file
 * @brief OnRISC Baltos Specific Board Initialization routines
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <envfs.h>
#include <environment.h>
#include <globalvar.h>
#include <linux/sizes.h>
#include <net.h>
#include <envfs.h>
#include <bootsource.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/am33xx-generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/gpmc.h>
#include <linux/err.h>
#include <mach/bbu.h>
#include <libfile.h>

static struct omap_barebox_part baltos_barebox_part = {
	.nand_offset = SZ_512K,
	.nand_size = 0x1e0000,
};

#ifndef CONFIG_OMAP_BUILD_IFT
struct bsp_vs_hwparam {
	uint32_t Magic;
	uint32_t HwRev;
	uint32_t SerialNumber;
	char PrdDate[11];
	uint16_t SystemId;
	uint8_t MAC1[6];
	uint8_t MAC2[6];
	uint8_t MAC3[6];
} __attribute__ ((packed));

static int baltos_read_eeprom(void)
{
	struct bsp_vs_hwparam hw_param;
	size_t size;
	char *buf, var_buf[32];
	int rc;
	unsigned char mac_addr[6];

	rc = read_file_2("/dev/eeprom0",
			 &size,
			 (void *)&buf,
			 sizeof(hw_param));
	if (rc && rc != -EFBIG)
		return rc;

	memcpy(&hw_param, buf, sizeof(hw_param));

	free(buf);

	if (hw_param.Magic == 0xDEADBEEF) {
		/* setup MAC1 */
		mac_addr[0] = hw_param.MAC1[0];
		mac_addr[1] = hw_param.MAC1[1];
		mac_addr[2] = hw_param.MAC1[2];
		mac_addr[3] = hw_param.MAC1[3];
		mac_addr[4] = hw_param.MAC1[4];
		mac_addr[5] = hw_param.MAC1[5];

		eth_register_ethaddr(0, mac_addr);

		/* setup MAC2 */
		mac_addr[0] = hw_param.MAC2[0];
		mac_addr[1] = hw_param.MAC2[1];
		mac_addr[2] = hw_param.MAC2[2];
		mac_addr[3] = hw_param.MAC2[3];
		mac_addr[4] = hw_param.MAC2[4];
		mac_addr[5] = hw_param.MAC2[5];

		eth_register_ethaddr(1, mac_addr);
	} else {
		printf("Baltos: incorrect magic number (0x%x) "
		       "in EEPROM\n",
		       hw_param.Magic);

		hw_param.SystemId = 0;
	}

	sprintf(var_buf, "%d", hw_param.SystemId);
	globalvar_add_simple("board.id", var_buf);

	return 0;
}
environment_initcall(baltos_read_eeprom);
#endif

static int baltos_devices_init(void)
{
	if (!of_machine_is_compatible("vscom,onrisc"))
		return 0;

	globalvar_add_simple("board.variant", "baltos");

	if (bootsource_get() == BOOTSOURCE_MMC)
		omap_set_bootmmc_devname("mmc0");

	omap_set_barebox_part(&baltos_barebox_part);

	if (IS_ENABLED(CONFIG_SHELL_NONE))
		return am33xx_of_register_bootdevice();

	return 0;
}
coredevice_initcall(baltos_devices_init);
