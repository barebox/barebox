/*
 * Copyright (C) 2004 Sascha Hauer, Synertronixx GmbH
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
#include <net.h>
#include <cfi_flash.h>

DECLARE_GLOBAL_DATA_PTR;

static struct cfi_platform_data cfi_info = {
};

static struct device_d cfi_dev = {
        .name     = "cfi_flash",
        .id       = "nor0",

        .map_base = 0x10000000,
        .size     = 16 * 1024 * 1024,

        .platform_data = &cfi_info,
};

static struct device_d sdram_dev = {
        .name     = "ram",
        .id       = "ram0",

        .map_base = 0x08000000,
        .size     = 16 * 1024 * 1024,
};

int board_init (void)
{
	gd->bd->bi_arch_number = MACH_TYPE_SCB9328;
	gd->bd->bi_boot_params = 0x08000100;

	return 0;
}

int dram_init (void)
{
	register_device(&cfi_dev);
	register_device(&sdram_dev);

#if ( CONFIG_NR_DRAM_BANKS > 0 )
	gd->bd->bi_dram[0].start = SCB9328_SDRAM_1;
	gd->bd->bi_dram[0].size = SCB9328_SDRAM_1_SIZE;
#endif
#if ( CONFIG_NR_DRAM_BANKS > 1 )
	gd->bd->bi_dram[1].start = SCB9328_SDRAM_2;
	gd->bd->bi_dram[1].size = SCB9328_SDRAM_2_SIZE;
#endif
#if ( CONFIG_NR_DRAM_BANKS > 2 )
	gd->bd->bi_dram[2].start = SCB9328_SDRAM_3;
	gd->bd->bi_dram[2].size = SCB9328_SDRAM_3_SIZE;
#endif
#if ( CONFIG_NR_DRAM_BANKS > 3 )
	gd->bd->bi_dram[3].start = SCB9328_SDRAM_4;
	gd->bd->bi_dram[3].size = SCB9328_SDRAM_4_SIZE;
#endif

#ifdef CONFIG_DRIVER_NET_DM9000
	eth_set_current(&dm9000_eth);
#endif

	return 0;
}

