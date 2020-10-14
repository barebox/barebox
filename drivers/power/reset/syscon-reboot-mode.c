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
#include <linux/overflow.h>

struct mode_reg {
	u32 offset;
	u32 mask;
};

struct syscon_reboot_mode {
	struct regmap *map;
	struct reboot_mode_driver reboot;
	struct mode_reg reg[];
};

static int syscon_reboot_mode_write(struct reboot_mode_driver *reboot,
				    const u32 *magic)
{
	struct syscon_reboot_mode *syscon_rbm;
	size_t i;
	int ret = 0;

	syscon_rbm = container_of(reboot, struct syscon_reboot_mode, reboot);

	for (i = 0; i < reboot->nelems; i++) {
		struct mode_reg *reg = &syscon_rbm->reg[i];

		ret = regmap_update_bits(syscon_rbm->map, reg->offset,
					 reg->mask, *magic++);
		if (ret < 0) {
			dev_err(reboot->dev, "update reboot mode bits failed\n");
			break;
		}
	}

	return ret;
}

static int syscon_reboot_mode_probe(struct device_d *dev)
{
	int ret, i, nelems;
	struct syscon_reboot_mode *syscon_rbm;
	struct reboot_mode_driver *reboot_template;
	struct device_node *np = dev->device_node;
	u32 *magic;

	nelems = of_property_count_elems_of_size(np, "offset", sizeof(__be32));
	if (nelems <= 0)
		return -EINVAL;

	syscon_rbm = xzalloc(struct_size(syscon_rbm, reg, nelems));

	ret = dev_get_drvdata(dev, (const void **)&reboot_template);
	if (ret)
		return ret;

	syscon_rbm->reboot = *reboot_template;
	syscon_rbm->reboot.dev = dev;

	syscon_rbm->map = syscon_node_to_regmap(dev->parent->device_node);
	if (IS_ERR(syscon_rbm->map))
		return PTR_ERR(syscon_rbm->map);

	magic = xzalloc(nelems * sizeof(*magic));

	for (i = 0; i < nelems; i++) {
		struct mode_reg *reg = &syscon_rbm->reg[i];

		ret = of_property_read_u32_index(np, "offset", i, &reg->offset);
		if (ret)
			goto free_magic;

		reg->mask = 0xffffffff;
		of_property_read_u32_index(np, "mask", i, &reg->mask);

		ret = regmap_read(syscon_rbm->map, reg->offset, &magic[i]);
		if (ret) {
			dev_err(dev, "error reading reboot mode: %s\n",
				strerror(-ret));
			goto free_magic;
		}

		magic[i] &= reg->mask;
	}

	ret = reboot_mode_register(&syscon_rbm->reboot, magic, nelems);
	if (ret)
		dev_err(dev, "can't register reboot mode\n");

free_magic:
	free(magic);
	return ret;

}

static struct reboot_mode_driver reboot_fixup  = {
	.write = syscon_reboot_mode_write,
	.priority = 100,
	.no_fixup = false,
};

static struct reboot_mode_driver reboot_nofixup  = {
	.write = syscon_reboot_mode_write,
	.priority = 50,
	.no_fixup = true,
};

static const struct of_device_id syscon_reboot_mode_of_match[] = {
	{ .compatible = "syscon-reboot-mode", .data = &reboot_fixup },
	{ .compatible = "barebox,syscon-reboot-mode", .data = &reboot_nofixup },
	{ /* sentinel */ }
};

static struct driver_d syscon_reboot_mode_driver = {
	.probe = syscon_reboot_mode_probe,
	.name = "syscon-reboot-mode",
	.of_compatible = syscon_reboot_mode_of_match,
};
coredevice_platform_driver(syscon_reboot_mode_driver);
