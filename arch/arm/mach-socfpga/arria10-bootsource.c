/*
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
 */

#include <common.h>
#include <bootsource.h>
#include <init.h>
#include <io.h>
#include <mach/arria10-system-manager.h>

static int arria10_boot_save_loc(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	uint32_t val;

	val = readl(ARRIA10_SYSMGR_BOOTINFO);

	switch ((val & 0x7000) >> 12) {
	case 0:
		/* reserved */
		break;
	case 1:
		/* FPGA, currently not decoded */
		break;
	case 2:
	case 3:
		src = BOOTSOURCE_NAND;
		break;
	case 4:
	case 5:
		src = BOOTSOURCE_MMC;
		break;
	case 6:
	case 7:
		src = BOOTSOURCE_SPI;
		break;
	}

	bootsource_set(src);
	bootsource_set_instance(0);

	return 0;
}
core_initcall(arria10_boot_save_loc);
