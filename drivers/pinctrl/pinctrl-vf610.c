/*
 * Copyright (c) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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
#include <of.h>
#include <pinctrl.h>
#include <malloc.h>
#include <gpio.h>

#include <mach/iomux-vf610.h>

enum {
	PINCTRL_VF610_MUX_LINE_SIZE = 20,

	PINCTRL_VF610_IBE = BIT(0),
	PINCTRL_VF610_OBE = BIT(1),
	PINCTRL_VF610_xBE = PINCTRL_VF610_OBE | PINCTRL_VF610_IBE,
};

struct pinctrl_vf610 {
	void __iomem *base;
	struct pinctrl_device pinctrl;
};

static int pinctrl_vf610_set_state(struct pinctrl_device *pdev,
				   struct device_node *np)
{
	const __be32 *list;
	int npins, size, i;

	struct pinctrl_vf610 *iomux =
		container_of(pdev, struct pinctrl_vf610, pinctrl);

	list = of_get_property(np, "fsl,pins", &size);
	if (!list)
		return -EINVAL;

	if (!size || size % PINCTRL_VF610_MUX_LINE_SIZE) {
		dev_err(pdev->dev, "Invalid fsl,pins property in %s\n",
			np->full_name);
		return -EINVAL;
	}

	npins = size / PINCTRL_VF610_MUX_LINE_SIZE;

	for (i = 0; i < npins; i++) {
		iomux_v3_cfg_t pad;
		u32 mux_reg   = be32_to_cpu(*list++);
		u32 input_reg = be32_to_cpu(*list++);
		u32 mux_val   = be32_to_cpu(*list++);
		u32 input_val = be32_to_cpu(*list++);
		u32 conf_val  = be32_to_cpu(*list++);

		pad = IOMUX_PAD(mux_reg, mux_reg, mux_val,
				input_reg, input_val, conf_val);

		vf610_setup_pad(iomux->base, pad);
	}

	return 0;
}

static int pinctrl_vf610_set_direction(struct pinctrl_device *pdev,
				       unsigned int pin, bool input)
{
	u32 pad_cr;
	const u32 off = pin * sizeof(u32);
	struct pinctrl_vf610 *iomux =
		container_of(pdev, struct pinctrl_vf610, pinctrl);

	pad_cr = readl(iomux->base + off);

	if (input) {
		pad_cr |= PINCTRL_VF610_IBE;
		pad_cr &= ~PINCTRL_VF610_OBE;
	} else {
		pad_cr &= ~PINCTRL_VF610_IBE;
		pad_cr |= PINCTRL_VF610_OBE;
	}

	writel(pad_cr, iomux->base + off);

	return 0;
}

static int pinctrl_vf610_get_direction(struct pinctrl_device *pdev,
				       unsigned int pin)
{
	const u32 off = pin * sizeof(u32);
	struct pinctrl_vf610 *iomux =
		container_of(pdev, struct pinctrl_vf610, pinctrl);

	const u32 pad_cr = readl(iomux->base + off);

	switch (pad_cr & PINCTRL_VF610_xBE) {
	case PINCTRL_VF610_IBE:
		return GPIOF_DIR_IN;
	case PINCTRL_VF610_OBE:
		return GPIOF_DIR_OUT;
	default:
		return -EINVAL;
	}
}

static struct pinctrl_ops pinctrl_vf610_ops = {
	.set_state = pinctrl_vf610_set_state,
	.set_direction = pinctrl_vf610_set_direction,
	.get_direction = pinctrl_vf610_get_direction,
};

static int pinctrl_vf610_probe(struct device_d *dev)
{
	int ret;
	struct resource *io;
	struct pinctrl_vf610 *iomux;

	iomux = xzalloc(sizeof(*iomux));

	io = dev_request_mem_resource(dev, 0);
	if (IS_ERR(io))
		return PTR_ERR(io);

	iomux->base = IOMEM(io->start);
	iomux->pinctrl.dev = dev;
	iomux->pinctrl.ops = &pinctrl_vf610_ops;
	iomux->pinctrl.base = 0;
	iomux->pinctrl.npins = ARCH_NR_GPIOS;

	ret = pinctrl_register(&iomux->pinctrl);
	if (ret)
		free(iomux);

	return ret;
}

static __maybe_unused struct of_device_id pinctrl_vf610_dt_ids[] = {
	{ .compatible = "fsl,vf610-iomuxc", },
	{ /* sentinel */ }
};

static struct driver_d pinctrl_vf610_driver = {
	.name		= "vf610-pinctrl",
	.probe		= pinctrl_vf610_probe,
	.of_compatible	= DRV_OF_COMPAT(pinctrl_vf610_dt_ids),
};

static int pinctrl_vf610_init(void)
{
	return platform_driver_register(&pinctrl_vf610_driver);
}
postcore_initcall(pinctrl_vf610_init);
