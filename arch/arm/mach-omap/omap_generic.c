/*
 * (C) Copyright 2013 Teresa Gámez, Phytec Messtechnik GmbH
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
 */
#include <common.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <fs.h>
#include <linux/stat.h>
#include <mach/generic.h>

enum omap_boot_src omap_bootsrc(void)
{
#if defined(CONFIG_ARCH_OMAP3)
	return omap3_bootsrc();
#elif defined(CONFIG_ARCH_OMAP4)
	return omap4_bootsrc();
#elif defined(CONFIG_ARCH_AM33XX)
	return am33xx_bootsrc();
#endif
}

#if defined(CONFIG_DEFAULT_ENVIRONMENT) && defined(CONFIG_MCI_STARTUP)
static int omap_env_init(void)
{
	struct stat s;
	char *diskdev = "/dev/disk0.0";
	int ret;

	if (omap_bootsrc() != OMAP_BOOTSRC_MMC1)
		return 0;

	ret = stat(diskdev, &s);
	if (ret) {
		printf("no %s. using default env\n", diskdev);
		return 0;
	}

	mkdir("/boot", 0666);
	ret = mount(diskdev, "fat", "/boot");
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	if (IS_ENABLED(CONFIG_OMAP_BUILD_IFT))
		default_environment_path = "/dev/defaultenv";
	else
		default_environment_path = "/boot/barebox.env";

	return 0;
}
late_initcall(omap_env_init);
#endif
