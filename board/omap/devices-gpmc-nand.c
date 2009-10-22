/**
 * @file
 * @brief GPMC specific NAND devices
 *
 * FileName: board/omap/devices-gpmc-nand.c
 *
 * GPMC NAND Devices such as those from Micron, Samsung are listed here
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
#include <asm/io.h>

#include <mach/silicon.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>

#ifdef CONFIG_MACH_OMAP_GPMC_GENERICNAND

#define GPMC_CONF1_VALx8	0x00000800
#define GPMC_CONF1_VALx16	0x00001800
/* Set up the generic params */
#ifdef CONFIG_MACH_OMAP_GPMC_GENERICNAND_LP_X8
#define GPMC_NAND_ECC_LAYOUT	GPMC_NAND_ECC_LP_x8_LAYOUT
#define GPMC_CONF1_VAL		GPMC_CONF1_VALx8
#define GPMC_NAND_DEV_WIDTH	8
#endif

#ifdef CONFIG_MACH_OMAP_GPMC_GENERICNAND_LP_X16
#define GPMC_NAND_ECC_LAYOUT	GPMC_NAND_ECC_LP_x16_LAYOUT
#define GPMC_CONF1_VAL		GPMC_CONF1_VALx16
#define GPMC_NAND_DEV_WIDTH	16
#endif

#ifdef CONFIG_MACH_OMAP_GPMC_GENERICNAND_SP_X8
#define GPMC_NAND_ECC_LAYOUT	GPMC_NAND_ECC_SP_x8_LAYOUT
#define GPMC_CONF1_VAL		GPMC_CONF1_VALx8
#define GPMC_NAND_DEV_WIDTH	8
#endif

#ifdef CONFIG_MACH_OMAP_GPMC_GENERICNAND_SP_X16
#define GPMC_NAND_ECC_LAYOUT	GPMC_NAND_ECC_SP_x16_LAYOUT
#define GPMC_CONF1_VAL		GPMC_CONF1_VALx16
#define GPMC_NAND_DEV_WIDTH	16
#endif

#ifdef CONFIG_NAND_OMAP_GPMC_HWECC
/**
 * The following is the organization of ECC as expected by BootROM.
 * Based on the type of device, this can vary. select the config accordingly
 */
static struct nand_ecclayout boot_rom_ecc = GPMC_NAND_ECC_LAYOUT;
#endif

/** GPMC timing for our nand device */
static struct gpmc_config nand_cfg = {
	.cfg = {
		GPMC_CONF1_VAL,	/*CONF1 */
		0x00141400,	/*CONF2 */
		0x00141400,	/*CONF3 */
		0x0F010F01,	/*CONF4 */
		0x010C1414,	/*CONF5 */
#ifdef CONFIG_ARCH_OMAP3
		/* Additional bits in OMAP3 */
		0x1F040000 |
#endif
		0x00000A80,	/*CONF6 */
		},

	/* Nand: dont care about base address */
	.base = 0x28000000,
	/* GPMC address map as small as possible */
	.size = GPMC_SIZE_16M,
};

/** NAND platform specific settings settings */
static struct gpmc_nand_platform_data nand_plat = {
	.cs = CONFIG_MACH_OMAP_CS,
	.device_width = GPMC_NAND_DEV_WIDTH,
	.max_timeout = MSECOND,
	.wait_mon_pin = 0,
#ifdef CONFIG_NAND_OMAP_GPMC_HWECC
	.plat_options = NAND_HWECC_ENABLE,
	.oob = &boot_rom_ecc,
#else
	.plat_options = NAND_HWECC_DISABLE,
#endif
	.priv = (void *)&nand_cfg,
};

/** NAND device definition */
static struct device_d gpmc_generic_nand_nand_device = {
	.name = "gpmc_nand",
	.map_base = OMAP_GPMC_BASE,
	.size = 1024 * 4,	/* GPMC size */
	.platform_data = (void *)&nand_plat,
};

/**
 * @brief gpmc_generic_nand_devices_init - init generic nand device
 *
 * @return success/fail based on device funtion
 */
static int gpmc_generic_nand_devices_init(void)
{
	/* Configure GPMC CS before register */
	gpmc_cs_config(nand_plat.cs, &nand_cfg);
	return register_device(&gpmc_generic_nand_nand_device);
}

device_initcall(gpmc_generic_nand_devices_init);

#endif				/* CONFIG_MACH_OMAP_GPMC_GENERICNAND */

/* All specific optimized nand configurations for GPMC NAND comes here */

