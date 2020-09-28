// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016, Fuzhou Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <of.h>
#include <regmap.h>
#include <mfd/syscon.h>
#include <linux/reboot-mode.h>

struct syscon_reboot_mode {
	struct regmap *map;
	struct reboot_mode_driver reboot;
	u32 offset;
	u32 mask;
};

static int syscon_reboot_mode_write(struct reboot_mode_driver *reboot,
				    unsigned int magic)
{
	struct syscon_reboot_mode *syscon_rbm;
	int ret;

	syscon_rbm = container_of(reboot, struct syscon_reboot_mode, reboot);

	ret = regmap_update_bits(syscon_rbm->map, syscon_rbm->offset,
				 syscon_rbm->mask, magic);
	if (ret < 0)
		dev_err(reboot->dev, "update reboot mode bits failed\n");

	return ret;
}

static int syscon_reboot_mode_probe(struct device_d *dev)
{
	int ret;
	struct syscon_reboot_mode *syscon_rbm;
	struct device_node *np = dev->device_node;
	u32 magic;

	syscon_rbm = xzalloc(sizeof(*syscon_rbm));

	syscon_rbm->reboot.dev = dev;
	syscon_rbm->reboot.write = syscon_reboot_mode_write;
	syscon_rbm->mask = 0xffffffff;

	syscon_rbm->map = syscon_node_to_regmap(dev->parent->device_node);
	if (IS_ERR(syscon_rbm->map))
		return PTR_ERR(syscon_rbm->map);

	if (of_property_read_u32(np, "offset", &syscon_rbm->offset))
		return -EINVAL;

	of_property_read_u32(np, "mask", &syscon_rbm->mask);

	ret = regmap_read(syscon_rbm->map, syscon_rbm->offset, &magic);
	if (ret) {
		dev_err(dev, "error reading reboot mode: %s\n",
			strerror(-ret));
		return ret;
	}

	magic &= syscon_rbm->mask;

	ret = reboot_mode_register(&syscon_rbm->reboot, magic);
	if (ret)
		dev_err(dev, "can't register reboot mode\n");

	return ret;
}

static const struct of_device_id syscon_reboot_mode_of_match[] = {
	{ .compatible = "syscon-reboot-mode" },
	{ /* sentinel */ }
};

static struct driver_d syscon_reboot_mode_driver = {
	.probe = syscon_reboot_mode_probe,
	.name = "syscon-reboot-mode",
	.of_compatible = syscon_reboot_mode_of_match,
};
coredevice_platform_driver(syscon_reboot_mode_driver);
