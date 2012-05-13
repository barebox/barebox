/**
 * @file
 * @brief GPMC specific NAND devices
 *
 * FileName: arch/arm/boards/omap/devices-gpmc-nand.c
 *
 */
/*
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
#include <console.h>
#include <init.h>
#include <driver.h>
#include <clock.h>
#include <io.h>

#include <mach/silicon.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>

#define GPMC_CONF1_VALx8	0x00000800
#define GPMC_CONF1_VALx16	0x00001800

/** NAND platform specific settings settings */
static struct gpmc_nand_platform_data nand_plat = {
	.cs = 0,
	.max_timeout = MSECOND,
	.wait_mon_pin = 0,
};

/**
 * @brief gpmc_generic_nand_devices_init - init generic nand device
 *
 * @return success/fail based on device function
 */
int gpmc_generic_nand_devices_init(int cs, int width,
		enum gpmc_ecc_mode eccmode, struct gpmc_config *nand_cfg)
{
	nand_plat.cs = cs;

	if (width == 16)
		nand_cfg->cfg[0] = GPMC_CONF1_VALx16;
	else
		nand_cfg->cfg[0] = GPMC_CONF1_VALx8;

	nand_plat.device_width = width;
	nand_plat.ecc_mode = eccmode;
	nand_plat.priv = nand_cfg;

	/* Configure GPMC CS before register */
	gpmc_cs_config(nand_plat.cs, nand_cfg);

	add_generic_device("gpmc_nand", DEVICE_ID_DYNAMIC, NULL, OMAP_GPMC_BASE,
			1024 * 4, IORESOURCE_MEM, &nand_plat);

	return 0;
}
