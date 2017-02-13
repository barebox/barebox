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
#include <of.h>
#include <pinctrl.h>
#include <malloc.h>
#include <mach/iomux-v3.h>
#include <mach/generic.h>

struct imx_iomux_v3 {
	void __iomem *base;
	struct pinctrl_device pinctrl;
	unsigned int flags;
};

struct imx_iomux_v3_data {
	unsigned int flags;
};

static void __iomem *iomuxv3_base;

/*
 * configures a single pad in the iomuxer
 */
int mxc_iomux_v3_setup_pad(iomux_v3_cfg_t pad)
{
	if (!iomuxv3_base)
		return -EINVAL;

	imx_setup_pad(iomuxv3_base, pad);
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_v3_setup_pad);


int mxc_iomux_v3_setup_multiple_pads(const iomux_v3_cfg_t *pad_list, unsigned count)
{
	const iomux_v3_cfg_t *p = pad_list;
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
		dev_err(iomux->pinctrl.dev, "Invalid fsl,pins property in %s\n",
				np->full_name);
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

		if (conf_val & IMX_PAD_SION) {
			mux_val |= IOMUXC_CONFIG_SION;
			conf_val &= ~IMX_PAD_SION;
		}

		if (conf_val & IMX_DT_NO_PAD_CTL)
			conf_reg = 0;

		iomux_v3_setup_pad(iomux->base, iomux->flags,
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
	struct imx_iomux_v3_data *drvdata = NULL;
	int ret;

	dev_get_drvdata(dev, (const void **)&drvdata);
	iomux = xzalloc(sizeof(*iomux));

	iomux->base = base;

	iomux->pinctrl.dev = dev;
	iomux->pinctrl.ops = &imx_iomux_v3_ops;
	if (drvdata)
		iomux->flags = drvdata->flags;

	ret = pinctrl_register(&iomux->pinctrl);
	if (ret)
		free(iomux);

	return ret;
}

static int imx_iomux_v3_probe(struct device_d *dev)
{
	void __iomem *base;
	struct resource *iores;
	int ret = 0;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	if (!iomuxv3_base)
		/*
		 * Uh, this works only for the older controllers, not for
		 * i.MX7 which has two iomux controllers. i.MX7 based boards
		 * should not use mxc_iomux_v3_setup_pad anyway.
		 */
		iomuxv3_base = base;

	if (IS_ENABLED(CONFIG_PINCTRL) && dev->device_node)
		ret = imx_pinctrl_dt(dev, base);

	return ret;
}

static struct imx_iomux_v3_data imx_iomux_imx7_lpsr_data = {
	.flags = ZERO_OFFSET_VALID | IMX7_PINMUX_LPSR,
};

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
	}, {
		.compatible = "fsl,imx6dl-iomuxc",
	}, {
		.compatible = "fsl,imx6sx-iomuxc",
	}, {
		.compatible = "fsl,imx6ul-iomuxc",
	}, {
		.compatible = "fsl,imx6sl-iomuxc",
	}, {
		.compatible = "fsl,imx7d-iomuxc",
	}, {
		.compatible = "fsl,imx7d-iomuxc-lpsr",
		.data = &imx_iomux_imx7_lpsr_data,
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
