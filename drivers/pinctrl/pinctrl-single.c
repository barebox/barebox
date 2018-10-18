/*
 * pinctrl-single - Generic device tree based pinctrl driver for one
 *                  register per pin type pinmux controllers
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <pinctrl.h>
#include <malloc.h>

struct pinctrl_single {
	void __iomem *base;
	struct pinctrl_device pinctrl;
	unsigned (*read)(void __iomem *reg);
	void (*write)(unsigned val, void __iomem *reg);
	unsigned width;
};

static unsigned __maybe_unused pcs_readb(void __iomem *reg)
{
	return readb(reg);
}

static unsigned __maybe_unused pcs_readw(void __iomem *reg)
{
	return readw(reg);
}

static unsigned __maybe_unused pcs_readl(void __iomem *reg)
{
	return readl(reg);
}

static void __maybe_unused pcs_writeb(unsigned val, void __iomem *reg)
{
	writeb(val, reg);
}

static void __maybe_unused pcs_writew(unsigned val, void __iomem *reg)
{
	writew(val, reg);
}

static void __maybe_unused pcs_writel(unsigned val, void __iomem *reg)
{
	writel(val, reg);
}

static int pcs_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct pinctrl_single *pcs = container_of(pdev, struct pinctrl_single, pinctrl);
	unsigned size = 0, index = 0;
	const __be32 *mux;

	dev_dbg(pcs->pinctrl.dev, "set state: %s\n", np->full_name);

	mux = of_get_property(np, "pinctrl-single,pins", &size);

	size /= sizeof(*mux);	/* Number of elements in array */

	if (!mux || !size || (size & 1)) {
		dev_err(pcs->pinctrl.dev, "bad data for mux %s\n",
			np->full_name);
		return -EINVAL;
	}

	while (index < size) {
		unsigned offset, val;

		offset = be32_to_cpup(mux + index++);
		val = be32_to_cpup(mux + index++);

		pcs->write(val, pcs->base + offset);
	}

	return 0;
}

static struct pinctrl_ops pcs_ops = {
	.set_state = pcs_set_state,
};

static int pcs_probe(struct device_d *dev)
{
	struct resource *iores;
	struct pinctrl_single *pcs;
	struct device_node *np = dev->device_node;
	int ret = 0;

	pcs = xzalloc(sizeof(*pcs));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	pcs->base = IOMEM(iores->start);
	pcs->pinctrl.dev = dev;
	pcs->pinctrl.ops = &pcs_ops;

	ret = of_property_read_u32(np, "pinctrl-single,register-width",
				   &pcs->width);
	if (ret) {
		dev_dbg(dev, "no pinctrl-single,register-width property\n");
		goto out;
	}

	switch (pcs->width) {
	case 8:
		pcs->read = pcs_readb;
		pcs->write = pcs_writeb;
		break;
	case 16:
		pcs->read = pcs_readw;
		pcs->write = pcs_writew;
		break;
	case 32:
		pcs->read = pcs_readl;
		pcs->write = pcs_writel;
		break;
	default:
		ret = -EINVAL;
		dev_dbg(dev, "invalid register width: %d\n", pcs->width);
		goto out;
	}

	ret = pinctrl_register(&pcs->pinctrl);
	if (ret)
		goto out;

	return 0;

out:
	free(pcs);

	return ret;
}

static __maybe_unused struct of_device_id pcs_dt_ids[] = {
	{
		.compatible = "pinctrl-single",
	}, {
		/* sentinel */
	}
};

static struct driver_d pcs_driver = {
	.name		= "pinctrl-single",
	.probe		= pcs_probe,
	.of_compatible	= DRV_OF_COMPAT(pcs_dt_ids),
};

static int pcs_init(void)
{
	return platform_driver_register(&pcs_driver);
}
core_initcall(pcs_init);
