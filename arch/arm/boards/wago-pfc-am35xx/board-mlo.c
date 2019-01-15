// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 WAGO Kontakttechnik GmbH & Co. KG <http://global.wago.com>
 * Author: Heinrich Toews <heinrich.toews@wago.com>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/sizes.h>
#include <mach/omap3-silicon.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>
#include <errno.h>
#include <mach/omap3-devices.h>
#include <mach/generic.h>

/* map first four erase blocks */
static struct omap_barebox_part pfc200_mlo_part = {
	/* start of boot0..boot3 (stage2 bootcode),
	 * we have 4x partitions
	 */
	.nand_offset = 4 * SZ_128K,
	.nand_size = 4 * SZ_128K,
};

/**
 * @brief Initialize the serial port to be used as console.
 *
 * @return result of device registration
 */
static int pfc200_init_console(void)
{
	barebox_set_model("Wago PFC200 MLO Stage #1");
	barebox_set_hostname("pfc200-mlo");

	omap3_add_uart3();

	return 0;
}
console_initcall(pfc200_init_console);

static int pfc200_mem_init(void)
{
	omap_add_ram0(SZ_256M);

	return 0;
}
mem_initcall(pfc200_mem_init);

static struct gpmc_nand_platform_data nand_plat = {
	.cs = 0,
	.device_width = 8,
	.ecc_mode = OMAP_ECC_BCH8_CODE_HW_ROMCODE,
	.nand_cfg = &omap3_nand_cfg,
};

static int pfc200_init_devices(void)
{
#ifdef CONFIG_OMAP_GPMC
	/*
	 * WP is made high and WAIT1 active Low
	 */
	gpmc_generic_init(0x10);
#endif
	omap_add_gpmc_nand_device(&nand_plat);
	omap_set_barebox_part(&pfc200_mlo_part);

	omap3_add_mmc1(NULL);

	return 0;
}
device_initcall(pfc200_init_devices);
