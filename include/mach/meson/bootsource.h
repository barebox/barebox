/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MACH_MESON_BOOTSOURCE_H
#define __MACH_MESON_BOOTSOURCE_H

/* the boot device ids the BootROM reports */
#define MESON_BOOT_DEVICE_RESERVED	0
#define MESON_BOOT_DEVICE_EMMC		1
#define MESON_BOOT_DEVICE_NAND		2
#define MESON_BOOT_DEVICE_SPI		3
#define MESON_BOOT_DEVICE_SD		4
#define MESON_BOOT_DEVICE_USB		5

void meson_bootsource_set(unsigned int boot_device);

#endif
