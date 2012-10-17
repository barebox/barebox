/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2008 by Sascha Hauer <kernel@pengutronix.de>
 * Copyright (C) 2009 by Jan Weitzel Phytec Messtechnik GmbH,
 *                       <armlinux@phytec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/iomux-v3.h>

static void __iomem *base;

/*
 * configures a single pad in the iomuxer
 */
int mxc_iomux_v3_setup_pad(iomux_v3_cfg_t pad)
{
	u32 mux_ctrl_ofs = (pad & MUX_CTRL_OFS_MASK) >> MUX_CTRL_OFS_SHIFT;
	u32 mux_mode = (pad & MUX_MODE_MASK) >> MUX_MODE_SHIFT;
	u32 sel_input_ofs = (pad & MUX_SEL_INPUT_OFS_MASK) >> MUX_SEL_INPUT_OFS_SHIFT;
	u32 sel_input = (pad & MUX_SEL_INPUT_MASK) >> MUX_SEL_INPUT_SHIFT;
	u32 pad_ctrl_ofs = (pad & MUX_PAD_CTRL_OFS_MASK) >> MUX_PAD_CTRL_OFS_SHIFT;
	u32 pad_ctrl = (pad & MUX_PAD_CTRL_MASK) >> MUX_PAD_CTRL_SHIFT;

	if (!base)
		return -EINVAL;

	debug("%s: mux 0x%08x -> 0x%04x pad: 0x%08x -> 0x%04x sel_inp: 0x%08x -> 0x%04x\n",
			__func__, mux_mode, mux_ctrl_ofs, pad_ctrl, pad_ctrl_ofs, sel_input,
			sel_input_ofs);

	if (mux_ctrl_ofs)
		__raw_writel(mux_mode, base + mux_ctrl_ofs);

	if (sel_input_ofs)
		__raw_writel(sel_input, base + sel_input_ofs);

	if (!(pad_ctrl & NO_PAD_CTRL) && pad_ctrl_ofs)
		__raw_writel(pad_ctrl, base + pad_ctrl_ofs);

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

static int imx_iomux_probe(struct device_d *dev)
{
	base = dev_request_mem_region(dev, 0);

	return 0;
}

static __maybe_unused struct of_device_id imx_iomux_dt_ids[] = {
	{
		.compatible = "fsl,imx35-iomux",
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_iomux_ids[] = {
	{
		.name = "imx35-iomux",
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_iomux_driver = {
	.name = "imx-iomuxv3",
	.probe = imx_iomux_probe,
	.of_compatible = DRV_OF_COMPAT(imx_iomux_dt_ids),
	.id_table = imx_iomux_ids,
};

static int imx_iomux_init(void)
{
	return platform_driver_register(&imx_iomux_driver);
}
postcore_initcall(imx_iomux_init);
