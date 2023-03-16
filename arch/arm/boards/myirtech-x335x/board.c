/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru> */

#include <bootsource.h>
#include <common.h>
#include <driver.h>
#include <envfs.h>
#include <init.h>
#include <linux/sizes.h>
#include <mach/omap/am33xx-generic.h>

static struct omap_barebox_part myir_barebox_part = {
	.nand_offset = SZ_128K * 4,
	.nand_size = SZ_1M,
};

static __init int myir_devices_init(void)
{
	if (!of_machine_is_compatible("myir,myc-am335x"))
		return 0;

	am33xx_register_ethaddr(0, 0);
	am33xx_register_ethaddr(1, 1);

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		omap_set_bootmmc_devname("mmc0");
		break;
	case BOOTSOURCE_NAND:
		omap_set_barebox_part(&myir_barebox_part);
		break;
	default:
		break;
	}

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT))
		defaultenv_append_directory(defaultenv_myirtech_x335x);

	if (IS_ENABLED(CONFIG_SHELL_NONE))
		return am33xx_of_register_bootdevice();

	return 0;
}
coredevice_initcall(myir_devices_init);
