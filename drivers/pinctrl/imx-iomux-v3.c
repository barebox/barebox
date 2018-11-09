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
#define SHARE_CONF_FSL_PIN_SIZE (FSL_PIN_SIZE - 1 * sizeof(u32))

#define IMX_DT_NO_PAD_CTL	(1 << 31)
#define IMX_PAD_SION		(1 << 30)

#define IOMUXC_CONFIG_SION	(0x1 << 4)

static int imx_iomux_v3_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct imx_iomux_v3 *iomux = container_of(pdev, struct imx_iomux_v3, pinctrl);
	const __be32 *list;
	const bool share_conf = iomux->flags & SHARE_CONF;
	int npins, size, i, fsl_pin_size;
	const char *name;
	u32 share_conf_val = 0;

	dev_dbg(iomux->pinctrl.dev, "set state: %s\n", np->full_name);

	if (share_conf) {
		u32 drive_strength, slew_rate;
		int ret;

		fsl_pin_size = SHARE_CONF_FSL_PIN_SIZE;
		name = "pinmux";

		ret = of_property_read_u32(np, "drive-strength",
					   &drive_strength);
		if (ret)
			return ret;

		ret = of_property_read_u32(np, "slew-rate", &slew_rate);
		if (ret)
			return ret;

		share_conf_val =
			FIELD_PREP(SHARE_CONF_PAD_CTL_DSE, drive_strength) |
			FIELD_PREP(SHARE_CONF_PAD_CTL_SRE, slew_rate);

		if (of_get_property(np, "drive-open-drain", NULL))
			share_conf_val |= SHARE_CONF_PAD_CTL_ODE;

		if (of_get_property(np, "input-schmitt-enable", NULL))
			share_conf_val |= SHARE_CONF_PAD_CTL_HYS;

		if (of_get_property(np, "input-enable", NULL))
			share_conf_val |= IMX_PAD_SION;

		if (of_get_property(np, "bias-pull-up", NULL))
			share_conf_val |= SHARE_CONF_PAD_CTL_PUE;
	} else {
		fsl_pin_size = FSL_PIN_SIZE;
		name = "fsl,pins";
	}

	list = of_get_property(np, name, &size);
	if (!list)
		return -EINVAL;

	if (!size || size % fsl_pin_size) {
		dev_err(iomux->pinctrl.dev, "Invalid fsl,pins property in %s\n",
				np->full_name);
		return -EINVAL;
	}

	npins = size / fsl_pin_size;

	for (i = 0; i < npins; i++) {
		u32 mux_reg = be32_to_cpu(*list++);
		u32 conf_reg = be32_to_cpu(*list++);
		u32 input_reg = be32_to_cpu(*list++);
		u32 mux_val = be32_to_cpu(*list++);
		u32 input_val = be32_to_cpu(*list++);
		u32 conf_val = share_conf ?
			share_conf_val : be32_to_cpu(*list++);

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

static struct imx_iomux_v3_data imx_iomux_imx8_data = {
	.flags = SHARE_CONF,
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
		.compatible = "fsl,imx8mq-iomuxc",
		.data = &imx_iomux_imx8_data,
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
core_initcall(imx_iomux_v3_init);
