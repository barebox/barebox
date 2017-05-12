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
#include <bootsource.h>
#include <envfs.h>
#include <bootm.h>
#include <init.h>
#include <io.h>
#include <fs.h>
#include <malloc.h>
#include <libfile.h>
#include <linux/stat.h>
#include <mach/gpmc.h>
#include <mach/generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/omap3-silicon.h>
#include <mach/omap4-silicon.h>
#include <mach/am33xx-generic.h>
#include <mach/omap3-generic.h>
#include <mach/omap4-generic.h>

void __iomem *omap_gpmc_base;

unsigned int __omap_cpu_type;

static void *omap_sram_start(void)
{
	if (cpu_is_am33xx())
		return (void *)AM33XX_SRAM0_START;
	if (cpu_is_omap3())
		return (void *)OMAP3_SRAM_BASE;
	if (cpu_is_omap4())
		return (void *)OMAP44XX_SRAM_BASE;
	return NULL;
}

static void *omap_scratch_space_start(void)
{
	if (cpu_is_am33xx())
		return (void *)AM33XX_SRAM_SCRATCH_SPACE;
	if (cpu_is_omap3())
		return (void *)OMAP3_SRAM_SCRATCH_SPACE;
	if (cpu_is_omap4())
		return (void *)OMAP44XX_SRAM_SCRATCH_SPACE;
	return NULL;
}

void __noreturn omap_start_barebox(void *barebox)
{
	int (*func)(void *) = barebox;
	void *sramadr = omap_sram_start();
	void *scratch = omap_scratch_space_start();

	memcpy(sramadr, scratch, sizeof(uint32_t) * 3);

	shutdown_barebox();
	func(sramadr);
	hang();
}

#ifdef CONFIG_BOOTM
static int do_bootm_omap_barebox(struct image_data *data)
{
	void (*barebox)(uint32_t);

	barebox = read_file(data->os_file, NULL);
	if (!barebox)
		return -EINVAL;

	if (data->dryrun) {
		free(barebox);
		return 0;
	}

	omap_start_barebox(barebox);
}

static struct image_handler omap_barebox_handler = {
	.name = "OMAP barebox",
	.bootm = do_bootm_omap_barebox,
	.filetype = filetype_arm_barebox,
};

static int omap_bootm_barebox(void)
{
	return register_image_handler(&omap_barebox_handler);
}
device_initcall(omap_bootm_barebox);
#endif

const static char *omap_bootmmc_dev;

void omap_set_bootmmc_devname(const char *devname)
{
	omap_bootmmc_dev = devname;
}

const char *omap_get_bootmmc_devname(void)
{
	return omap_bootmmc_dev;
}

#if defined(CONFIG_ENV_HANDLING)
static char *envpath = "/mnt/mmc0.0/barebox.env";

static int omap_env_init(void)
{
	int ret;
	const char *diskdev;
	char *partname;
	struct cdev *cdev;
	const char *rootpath;

	if (bootsource_get() != BOOTSOURCE_MMC)
		return 0;

	if (omap_bootmmc_dev)
		diskdev = omap_bootmmc_dev;
	else
		diskdev = "disk0";

	device_detect_by_name(diskdev);
	partname = basprintf("%s.0", diskdev);
	cdev = cdev_by_name(partname);
	if (cdev == NULL) {
		pr_err("Failed to get device %s\n", partname);
		goto out;
	}

	rootpath = cdev_mount_default(cdev, NULL);
	if (IS_ERR(rootpath)) {
		pr_err("Failed to load environment: mount %s failed (%d)\n",
						cdev->name, IS_ERR(rootpath));
		goto out;
	}
	ret = symlink(rootpath, "/boot");
	if (ret < 0)
		pr_warn("Failed to create symlink from %s to %s\n", rootpath
								, "/boot");

	envpath = basprintf("%s/barebox.env", rootpath);

	pr_debug("Loading default env from %s on device %s\n", envpath,
								partname);
	default_environment_path_set(envpath);

out:
	free(partname);

	return 0;
}
late_initcall(omap_env_init);
#endif

static int omap_soc_from_dt(void)
{
        if (of_machine_is_compatible("ti,am33xx"))
		return OMAP_CPU_AM33XX;
        if (of_machine_is_compatible("ti,omap4"))
		return OMAP_CPU_OMAP4;
        if (of_machine_is_compatible("ti,omap3"))
		return OMAP_CPU_OMAP3;

	return 0;
}

static int omap_init(void)
{
	int ret;
	struct device_node *root;

	root = of_get_root_node();
	if (root) {
		__omap_cpu_type = omap_soc_from_dt();
		if (!__omap_cpu_type)
			hang();
	}

	if (cpu_is_omap3())
		ret = omap3_init();
	else if (cpu_is_omap4())
		ret = omap4_init();
	else if (cpu_is_am33xx())
		ret = am33xx_init();
	else
		return -EINVAL;

	if (root)
		return ret;

	if (cpu_is_omap3())
		ret = omap3_devices_init();
	else if (cpu_is_omap4())
		ret = omap4_devices_init();
	else if (cpu_is_am33xx())
		ret = am33xx_devices_init();
	else
		return -EINVAL;

	return ret;
}
postcore_initcall(omap_init);
