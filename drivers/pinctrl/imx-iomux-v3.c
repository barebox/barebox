/*
 * imx-iomux-v3.c - i.MX iomux-v3 pinctrl support
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
#include <mach/iomux-v3.h>

struct imx_iomux_v3 {
	void __iomem *base;
	struct pinctrl_device pinctrl;
};

static void __iomem *iomuxv3_base;
static struct device_d *iomuxv3_dev;

static void imx_iomuxv3_setup_single(void __iomem *base, struct device_d *dev,
		u32 mux_reg, u32 conf_reg, u32 input_reg,
		u32 mux_val, u32 conf_val, u32 input_val)
{
	dev_dbg(dev,
		"mux: 0x%08x -> 0x%04x, conf: 0x%08x -> 0x%04x input: 0x%08x -> 0x%04x\n",
		mux_val, mux_reg, conf_val, conf_reg, input_val, input_reg);

	if (mux_reg)
		writel(mux_val, base + mux_reg);
	if (conf_reg)
		writel(conf_val, base + conf_reg);
	if (input_reg)
		writel(input_val, base + input_reg);
}

/*
 * configures a single pad in the iomuxer
 */
int mxc_iomux_v3_setup_pad(iomux_v3_cfg_t pad)
{
	u32 mux_reg = (pad & MUX_CTRL_OFS_MASK) >> MUX_CTRL_OFS_SHIFT;
	u32 mux_val = (pad & MUX_MODE_MASK) >> MUX_MODE_SHIFT;
	u32 input_reg = (pad & MUX_SEL_INPUT_OFS_MASK) >> MUX_SEL_INPUT_OFS_SHIFT;
	u32 input_val = (pad & MUX_SEL_INPUT_MASK) >> MUX_SEL_INPUT_SHIFT;
	u32 conf_reg = (pad & MUX_PAD_CTRL_OFS_MASK) >> MUX_PAD_CTRL_OFS_SHIFT;
	u32 conf_val = (pad & MUX_PAD_CTRL_MASK) >> MUX_PAD_CTRL_SHIFT;

	if (!iomuxv3_base)
		return -EINVAL;

	if (conf_val & NO_PAD_CTRL)
		conf_reg = 0;

	imx_iomuxv3_setup_single(iomuxv3_base, iomuxv3_dev,
			mux_reg, conf_reg, input_reg,
			mux_val, conf_val, input_val);

	return 0;
}
EXPORT_SYMBOL(mxc_iomux_v3_setup_pad);


int mxc_iomux_v3_setup_multiple_pads(iomux_v3_cfg_t *pad_list, unsigned count)
{
	iomux_v3_cfg_t *p = pad_list;
	int i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = mxc_iomux_v3_setup_pad(*p);
		if (ret)
			return ret;
		p++;
	}
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_v3_setup_multiple_pads);

/*
 * Each pin represented in fsl,pins consists of 5 u32 PIN_FUNC_ID and
 * 1 u32 CONFIG, so 24 types in total for each pin.
 */
#define FSL_PIN_SIZE 24

#define IMX_DT_NO_PAD_CTL	(1 << 31)
#define IMX_PAD_SION		(1 << 30)

#define IOMUXC_CONFIG_SION	(0x1 << 4)

static int imx_iomux_v3_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct imx_iomux_v3 *iomux = container_of(pdev, struct imx_iomux_v3, pinctrl);
	const __be32 *list;
	int npins, size, i;

	dev_dbg(iomux->pinctrl.dev, "set state: %s\n", np->full_name);

	list = of_get_property(np, "fsl,pins", &size);
	if (!list)
		return -EINVAL;


	if (!size || size % FSL_PIN_SIZE) {
		dev_err(iomux->pinctrl.dev, "Invalid fsl,pins property\n");
		return -EINVAL;
	}

	npins = size / FSL_PIN_SIZE;

	for (i = 0; i < npins; i++) {
		u32 mux_reg = be32_to_cpu(*list++);
		u32 conf_reg = be32_to_cpu(*list++);
		u32 input_reg = be32_to_cpu(*list++);
		u32 mux_val = be32_to_cpu(*list++);
		u32 input_val = be32_to_cpu(*list++);
		u32 conf_val = be32_to_cpu(*list++);

		if (conf_val & IMX_PAD_SION)
			mux_val |= IOMUXC_CONFIG_SION;

		if (conf_val & IMX_DT_NO_PAD_CTL)
			conf_reg = 0;

		imx_iomuxv3_setup_single(iomux->base, iomux->pinctrl.dev,
				mux_reg, conf_reg, input_reg,
				mux_val, conf_val, input_val);
	}

	return 0;
}

static struct pinctrl_ops imx_iomux_v3_ops = {
	.set_state = imx_iomux_v3_set_state,
};

static int imx_pinctrl_dt(struct device_d *dev, void __iomem *base)
{
	struct imx_iomux_v3 *iomux;
	int ret;

	iomux = xzalloc(sizeof(*iomux));

	iomux->base = base;

	iomux->pinctrl.dev = dev;
	iomux->pinctrl.ops = &imx_iomux_v3_ops;

	ret = pinctrl_register(&iomux->pinctrl);
	if (ret)
		free(iomux);

	return ret;
}

static int imx_iomux_v3_probe(struct device_d *dev)
{
	int ret = 0;

	if (iomuxv3_base)
		return -EBUSY;

	iomuxv3_base = dev_request_mem_region(dev, 0);
	iomuxv3_dev = dev;

	if (IS_ENABLED(CONFIG_PINCTRL) && dev->device_node)
		ret = imx_pinctrl_dt(dev, iomuxv3_base);

	return ret;
}

static __maybe_unused struct of_device_id imx_iomux_v3_dt_ids[] = {
	{
		.compatible = "fsl,imx25-iomuxc",
	}, {
		.compatible = "fsl,imx35-iomuxc",
	}, {
		.compatible = "fsl,imx51-iomuxc",
	}, {
		.compatible = "fsl,imx53-iomuxc",
	}, {
		.compatible = "fsl,imx6q-iomuxc",
	},  {
		.compatible = "fsl,imx6dl-iomuxc",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_iomux_v3_driver = {
	.name		= "imx-iomuxv3",
	.probe		= imx_iomux_v3_probe,
	.of_compatible	= DRV_OF_COMPAT(imx_iomux_v3_dt_ids),
};

static int imx_iomux_v3_init(void)
{
	return platform_driver_register(&imx_iomux_v3_driver);
}
postcore_initcall(imx_iomux_v3_init);
