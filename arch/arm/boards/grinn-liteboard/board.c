/*
 * Copyright (C) 2018 Grinn
 *
 * Author: Marcin Niestroj <m.niestroj@grinn-global.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#define pr_fmt(fmt) "liteboard: " fmt

#include <bootsource.h>
#include <common.h>
#include <envfs.h>
#include <init.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <malloc.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <of.h>

static void bbu_register_handler_sd(bool is_boot_source)
{
	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc0.barebox",
				is_boot_source ? BBU_HANDLER_FLAG_DEFAULT : 0);
}

static void bbu_register_handler_emmc(bool is_boot_source)
{
	int emmc_boot_flag = 0, emmc_flag = 0;
	const char *bootpart;
	struct device_d *dev;
	int ret;

	if (!is_boot_source)
		goto bbu_register;

	dev = get_device_by_name("mmc1");
	if (!dev) {
		pr_warn("Failed to get eMMC device\n");
		goto bbu_register;
	}

	ret = device_detect(dev);
	if (ret) {
		pr_warn("Failed to probe eMMC\n");
		goto bbu_register;
	}

	bootpart = dev_get_param(dev, "boot");
	if (!bootpart) {
		pr_warn("Failed to get eMMC boot configuration\n");
		goto bbu_register;
	}

	if (!strncmp(bootpart, "boot", 4))
		emmc_boot_flag |= BBU_HANDLER_FLAG_DEFAULT;
	else
		emmc_flag |= BBU_HANDLER_FLAG_DEFAULT;

bbu_register:
	imx6_bbu_internal_mmc_register_handler("emmc", "/dev/mmc1.barebox",
					emmc_flag);
	imx6_bbu_internal_mmcboot_register_handler("emmc-boot", "mmc1",
					emmc_boot_flag);
}

static const struct {
	const char *name;
	const char *env;
	void (*bbu_register_handler)(bool);
} boot_sources[] = {
	{"SD",   "/chosen/environment-sd",   bbu_register_handler_sd},
	{"eMMC", "/chosen/environment-emmc", bbu_register_handler_emmc},
};

static int liteboard_devices_init(void)
{
	int boot_source_idx = 0;
	int ret;
	int i;

	if (!of_machine_is_compatible("grinn,imx6ul-liteboard"))
		return 0;

	barebox_set_hostname("liteboard");

	if (bootsource_get() == BOOTSOURCE_MMC) {
		int mmc_idx = bootsource_get_instance();

		if (0 <= mmc_idx && mmc_idx < ARRAY_SIZE(boot_sources))
			boot_source_idx = mmc_idx;
	}

	ret = of_device_enable_path(boot_sources[boot_source_idx].env);
	if (ret < 0)
		pr_warn("Failed to enable environment partition '%s' (%d)\n",
			boot_sources[boot_source_idx].env, ret);

	pr_notice("Using environment in %s\n",
		boot_sources[boot_source_idx].name);

	for (i = 0; i < ARRAY_SIZE(boot_sources); i++)
		boot_sources[i].bbu_register_handler(boot_source_idx == i);

	return 0;
}
device_initcall(liteboard_devices_init);
