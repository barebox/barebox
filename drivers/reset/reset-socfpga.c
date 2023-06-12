// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2014 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * based on
 * Allwinner SoCs Reset Controller driver
 *
 * Copyright 2013 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#define BANK_INCREMENT		4
#define NR_BANKS		8

struct socfpga_reset_data {
	spinlock_t			lock;
	void __iomem			*membase;
	struct reset_controller_dev	rcdev;
};

static int socfpga_reset_assert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct socfpga_reset_data *data = container_of(rcdev,
						     struct socfpga_reset_data,
						     rcdev);
	int bank = id / BITS_PER_LONG;
	int offset = id % BITS_PER_LONG;
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&data->lock, flags);

	reg = readl(data->membase + (bank * BANK_INCREMENT));
	writel(reg | BIT(offset), data->membase + (bank * BANK_INCREMENT));
	spin_unlock_irqrestore(&data->lock, flags);

	return 0;
}

static int socfpga_reset_deassert(struct reset_controller_dev *rcdev,
				  unsigned long id)
{
	struct socfpga_reset_data *data = container_of(rcdev,
						     struct socfpga_reset_data,
						     rcdev);

	int bank = id / BITS_PER_LONG;
	int offset = id % BITS_PER_LONG;
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&data->lock, flags);

	reg = readl(data->membase + (bank * BANK_INCREMENT));
	writel(reg & ~BIT(offset), data->membase + (bank * BANK_INCREMENT));

	spin_unlock_irqrestore(&data->lock, flags);

	return 0;
}

static const struct reset_control_ops socfpga_reset_ops = {
	.assert		= socfpga_reset_assert,
	.deassert	= socfpga_reset_deassert,
};

static int socfpga_reset_probe(struct device *dev)
{
	struct socfpga_reset_data *data;
	struct resource *res;
	struct device_node *np = dev->of_node;
	u32 modrst_offset;

	data = xzalloc(sizeof(*data));

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	data->membase = IOMEM(res->start);

	if (of_property_read_u32(np, "altr,modrst-offset", &modrst_offset)) {
		dev_warn(dev, "missing altr,modrst-offset property, assuming 0x10!\n");
		modrst_offset = 0x10;
	}
	data->membase += modrst_offset;

	spin_lock_init(&data->lock);

	data->rcdev.nr_resets = NR_BANKS * BITS_PER_LONG;
	data->rcdev.ops = &socfpga_reset_ops;
	data->rcdev.of_node = np;

	return reset_controller_register(&data->rcdev);
}

static const struct of_device_id socfpga_reset_dt_ids[] = {
	{ .compatible = "altr,rst-mgr", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, socfpga_reset_dt_ids);

static struct driver socfpga_reset_driver = {
	.name = "socfpga_reset",
	.probe	= socfpga_reset_probe,
	.of_compatible	= DRV_OF_COMPAT(socfpga_reset_dt_ids),
};

postcore_platform_driver(socfpga_reset_driver);
