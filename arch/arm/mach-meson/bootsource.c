// SPDX-License-Identifier: GPL-2.0-only

#include <bootsource.h>
#include <mach/meson/bootsource.h>

/*
 * MMC instances are numbered after the sd_emmc_a/b/c controllers, the
 * barebox,bootsource-mmc* properties in /chosen map them to mmc aliases.
 */
void meson_bootsource_set(unsigned int boot_device)
{
	switch (boot_device) {
	case MESON_BOOT_DEVICE_EMMC:
		bootsource_set(BOOTSOURCE_MMC, 2);
		break;
	case MESON_BOOT_DEVICE_SD:
		bootsource_set(BOOTSOURCE_MMC, 1);
		break;
	case MESON_BOOT_DEVICE_NAND:
		bootsource_set(BOOTSOURCE_NAND, 0);
		break;
	case MESON_BOOT_DEVICE_SPI:
		bootsource_set(BOOTSOURCE_SPI_NOR, 0);
		break;
	case MESON_BOOT_DEVICE_USB:
		bootsource_set(BOOTSOURCE_USB, 0);
		break;
	default:
		bootsource_set(BOOTSOURCE_UNKNOWN, 0);
		break;
	}
}
