/*
 * pinctrl-mxs.c - MXS pinctrl support
 *
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <stmp-device.h>

struct mxs_pinctrl {
	void __iomem *base;
	struct pinctrl_device pinctrl;
};

#define PINID(bank, pin)	((bank) * 32 + (pin))
#define MUXID_TO_PINID(m)	PINID((m) >> 12 & 0xf, (m) >> 4 & 0xff)
#define MUXID_TO_MUXSEL(m)	((m) & 0xf)
#define PINID_TO_BANK(p)	((p) >> 5)
#define PINID_TO_PIN(p)		((p) % 32)

static int mxs_pinctrl_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct mxs_pinctrl *iomux = container_of(pdev, struct mxs_pinctrl, pinctrl);
	const __be32 *list;
	int npins, size, i;
	u32 val, ma, vol, pull;
	int ret;
	int ma_present = 0, vol_present = 0, pull_present = 0;

	dev_dbg(iomux->pinctrl.dev, "set state: %s\n", np->full_name);

	list = of_get_property(np, "fsl,pinmux-ids", &size);
	if (!list)
		return -EINVAL;

	if (!size || size % 4) {
		dev_err(iomux->pinctrl.dev, "Invalid fsl,pinmux-ids property in %s\n",
				np->full_name);
		return -EINVAL;
	}

	npins = size / 4;

	ret = of_property_read_u32(np, "fsl,drive-strength", &ma);
	if (!ret)
		ma_present = 1;

	ret = of_property_read_u32(np, "fsl,voltage", &vol);
	if (!ret)
		vol_present = 1;

	ret = of_property_read_u32(np, "fsl,pull-up", &pull);
	if (!ret)
		pull_present = 1;

	for (i = 0; i < npins; i++) {
		int muxsel, pinid, bank, pin, shift;
		void __iomem *reg;

		val = be32_to_cpu(*list++);

		muxsel = MUXID_TO_MUXSEL(val);
		pinid = MUXID_TO_PINID(val);

		bank = PINID_TO_BANK(pinid);
		pin = PINID_TO_PIN(pinid);
		reg = iomux->base + 0x100;
		reg += bank * 0x20 + pin / 16 * 0x10;
		shift = pin % 16 * 2;

		writel(0x3 << shift, reg + STMP_OFFSET_REG_CLR);
		writel(muxsel << shift, reg + STMP_OFFSET_REG_SET);

		dev_dbg(iomux->pinctrl.dev, "(val: 0x%x) pin %d, mux %d, ma: %d, vol: %d, pull: %d\n",
				val, pinid, muxsel, ma, vol, pull);

		/* drive */
		reg = iomux->base + 0x300;
		reg += bank * 0x40 + pin / 8 * 0x10;

		/* mA */
		if (ma_present) {
			shift = pin % 8 * 4;
			writel(0x3 << shift, reg + STMP_OFFSET_REG_CLR);
			writel(ma << shift, reg + STMP_OFFSET_REG_SET);
		}

		/* vol */
		if (vol_present) {
			shift = pin % 8 * 4 + 2;
			if (vol)
				writel(1 << shift, reg + STMP_OFFSET_REG_SET);
			else
				writel(1 << shift, reg + STMP_OFFSET_REG_CLR);
		}

		/* pull */
		if (pull_present) {
			reg = iomux->base + 0x600;
			reg += bank * 0x10;
			shift = pin;
			if (pull)
				writel(1 << shift, reg + STMP_OFFSET_REG_SET);
			else
				writel(1 << shift, reg + STMP_OFFSET_REG_CLR);
		}
	}

	return 0;
}

static struct pinctrl_ops mxs_pinctrl_ops = {
	.set_state = mxs_pinctrl_set_state,
};

static int mxs_pinctrl_probe(struct device_d *dev)
{
	struct mxs_pinctrl *iomux;
	int ret = 0;

	iomux = xzalloc(sizeof(*iomux));

	iomux->base = dev_get_mem_region(dev, 0);

	iomux->pinctrl.dev = dev;
	iomux->pinctrl.ops = &mxs_pinctrl_ops;

	ret = pinctrl_register(&iomux->pinctrl);
	if (ret)
		free(iomux);

	return ret;
}

static __maybe_unused struct of_device_id mxs_pinctrl_dt_ids[] = {
	{
		.compatible = "fsl,imx28-pinctrl",
	}, {
		/* sentinel */
	}
};

static struct driver_d mxs_pinctrl_driver = {
	.name		= "mxs-pinctrl",
	.probe		= mxs_pinctrl_probe,
	.of_compatible	= DRV_OF_COMPAT(mxs_pinctrl_dt_ids),
};

static int mxs_pinctrl_init(void)
{
	return platform_driver_register(&mxs_pinctrl_driver);
}
postcore_initcall(mxs_pinctrl_init);
