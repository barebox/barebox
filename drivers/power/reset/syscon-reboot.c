// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Generic Syscon Reboot Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Author: Feng Kan <fkan@apm.com>
 */
#include <common.h>
#include <init.h>
#include <restart.h>
#include <mfd/syscon.h>

struct syscon_reboot_context {
	struct regmap *map;
	u32 offset;
	u32 value;
	u32 mask;
	struct restart_handler restart_handler;
};

static void __noreturn syscon_restart_handle(struct restart_handler *this)
{
	struct syscon_reboot_context *ctx =
			container_of(this, struct syscon_reboot_context,
					restart_handler);

	/* Issue the reboot */
	regmap_update_bits(ctx->map, ctx->offset, ctx->mask, ctx->value);

	mdelay(1000);

	panic("Unable to restart system\n");
}

static int syscon_reboot_probe(struct device *dev)
{
	struct syscon_reboot_context *ctx;
	int mask_err, value_err;
	int err;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->map = syscon_regmap_lookup_by_phandle(dev->of_node, "regmap");
	if (IS_ERR(ctx->map)) {
		ctx->map = syscon_node_to_regmap(dev->parent->of_node);
		if (IS_ERR(ctx->map))
			return PTR_ERR(ctx->map);
	}

	if (of_property_read_u32(dev->of_node, "offset", &ctx->offset))
		return -EINVAL;

	value_err = of_property_read_u32(dev->of_node, "value", &ctx->value);
	mask_err = of_property_read_u32(dev->of_node, "mask", &ctx->mask);
	if (value_err && mask_err) {
		dev_err(dev, "unable to read 'value' and 'mask'");
		return -EINVAL;
	}

	if (value_err) {
		/* support old binding */
		ctx->value = ctx->mask;
		ctx->mask = 0xFFFFFFFF;
	} else if (mask_err) {
		/* support value without mask*/
		ctx->mask = 0xFFFFFFFF;
	}

	ctx->restart_handler.name = "syscon-reboot";
	ctx->restart_handler.restart = syscon_restart_handle;
	ctx->restart_handler.priority = 192;
	ctx->restart_handler.of_node = dev->of_node;

	err = restart_handler_register(&ctx->restart_handler);
	if (err)
		dev_err(dev, "can't register restart notifier\n");

	return err;
}

static const struct of_device_id syscon_reboot_of_match[] = {
	{ .compatible = "syscon-reboot" },
	{}
};
MODULE_DEVICE_TABLE(of, syscon_reboot_of_match);

static struct driver syscon_reboot_driver = {
	.probe = syscon_reboot_probe,
	.name = "syscon-reboot",
	.of_compatible = syscon_reboot_of_match,
};
coredevice_platform_driver(syscon_reboot_driver);
