/*
 * pinctrl-single - Generic device tree based pinctrl driver for one
 *                  register per pin type pinmux controllers
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
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
	unsigned int width;
	unsigned int fmask;
	unsigned int fshift;
	unsigned int fmax;

	bool bits_per_mux;
	unsigned int bits_per_pin;
	unsigned int args_count;
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
	unsigned int offset, val, rows, mask, reg, i;
	const __be32 *mux;

	dev_dbg(pcs->pinctrl.dev, "set state: %s\n", np->full_name);
	if (pcs->bits_per_mux) {
		mux = of_get_property(np, "pinctrl-single,bits", &size);
		if (size % 3 != 0)
			dev_err(pcs->pinctrl.dev,
				"invalid args_count for spec: %u\n", size);

		size /= sizeof(*mux);	/* Number of elements in array */
		rows = size / 3;

		for (i = 0; i < rows; i++) {
			offset = be32_to_cpup(mux + index++);
			val = be32_to_cpup(mux + index++);
			mask = be32_to_cpup(mux + index++);
			reg = pcs->read(pcs->base + offset);
			reg &= ~mask;
			reg |= val;
			pcs->write(reg, pcs->base + offset);
		}
	} else {
		mux = of_get_property(np, "pinctrl-single,pins", &size);

		size /= sizeof(*mux);	/* Number of elements in array */

		if (!mux || !size || (size % (pcs->args_count + 1))) {
			dev_err(pcs->pinctrl.dev, "bad data for mux %s\n",
				np->full_name);
			return -EINVAL;
		}

		while (index < size) {
			unsigned int offset, val;

			offset = be32_to_cpup(mux + index++);
			val = be32_to_cpup(mux + index++);
			if (pcs->args_count > 1) {
				/* If a 2nd data cell is present, it's ORed into
				 * the 1st.  Additional cells are undefined,
				 * just skip them.
				 */
				val |= be32_to_cpup(mux + index);
				index += pcs->args_count - 1;
			}

			pcs->write(val, pcs->base + offset);
		}
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

	ret = of_property_read_u32(np, "pinctrl-single,function-mask",
				   &pcs->fmask);
	if (!ret) {
		pcs->fshift = __ffs(pcs->fmask);
		pcs->fmax = pcs->fmask >> pcs->fshift;
	} else {
		/* If mask property doesn't exist, function mux is invalid. */
		pcs->fmask = 0;
		pcs->fshift = 0;
		pcs->fmax = 0;
	}

	pcs->bits_per_mux =
		of_property_read_bool(np, "pinctrl-single,bit-per-mux");
	if (pcs->bits_per_mux)
		pcs->bits_per_pin = fls(pcs->fmask);

	/* If no pinctrl-cells is present, default to old style of 2 cells with
	 * bits per mux and 1 cell otherwise.
	 */
	ret = of_property_read_u32(np, "#pinctrl-cells", &pcs->args_count);
	if (ret)
		pcs->args_count = pcs->bits_per_mux ? 2 : 1;

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

core_platform_driver(pcs_driver);
