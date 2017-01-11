/*
 * i.MX drm driver - parallel display implementation
 *
 * Copyright (C) 2016 Philippe Leduc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <fb.h>
#include <io.h>
#include <of_graph.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <video/vpl.h>
#include <video/media-bus-format.h>
#include <linux/err.h>

#include "imx-ipu-v3.h"

struct imx_pd {
	struct device_d *dev;
	struct display_timings *timings;
	u32 bus_format;
	struct vpl vpl;
};

static int imx_pd_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *data)
{
	struct imx_pd *imx_pd = container_of(vpl, struct imx_pd, vpl);
	struct ipu_di_mode *mode;
	struct display_timings *timings;

	switch (cmd) {
	case IMX_IPU_VPL_DI_MODE:
		mode = data;

		mode->di_clkflags = IPU_DI_CLKMODE_SYNC;
		mode->bus_format = imx_pd->bus_format;
		return 0;

	case VPL_GET_VIDEOMODES:
		timings = data;

		timings->num_modes   = imx_pd->timings->num_modes;
		timings->native_mode = imx_pd->timings->native_mode;
		timings->modes       = imx_pd->timings->modes;
		timings->edid        = NULL;
		return 0;
	}

	return 0;
}

static int imx_pd_probe(struct device_d *dev)
{
	struct device_node *node = dev->device_node;
	struct imx_pd *imx_pd;
	const char *fmt;
	int ret;

	imx_pd = xzalloc(sizeof(*imx_pd));
	imx_pd->dev = dev;

	ret = of_property_read_string(node, "interface-pix-fmt", &fmt);
	if (!ret) {
		if (!strcmp(fmt, "rgb24"))
			imx_pd->bus_format = MEDIA_BUS_FMT_RGB888_1X24;
		else if (!strcmp(fmt, "rgb565"))
			imx_pd->bus_format = MEDIA_BUS_FMT_RGB565_1X16;
		else if (!strcmp(fmt, "bgr666"))
			imx_pd->bus_format = MEDIA_BUS_FMT_RGB666_1X18;
		else {
			dev_err(dev, "invalid interface-pix-fmt\n");
			return -EINVAL;
		}
	}

	imx_pd->timings = of_get_display_timings(node);
	if (!imx_pd->timings) {
		dev_err(dev, "No display timings panel found\n");
		return -EINVAL;
	}

	imx_pd->vpl.node = node;
	imx_pd->vpl.ioctl = &imx_pd_ioctl;
	ret = vpl_register(&imx_pd->vpl);
	if (ret)
		return ret;

	return 0;
}

static struct of_device_id imx_pd_dt_ids[] = {
	{ .compatible = "fsl,imx-parallel-display", },
	{ /* sentinel */ }
};

static struct driver_d imx_pd_driver = {
	.probe			  = imx_pd_probe,
	.of_compatible	= imx_pd_dt_ids,
	.name				= "imx-parallel-display",
};
device_platform_driver(imx_pd_driver);

MODULE_DESCRIPTION("i.MX Parallel display driver");
MODULE_AUTHOR("Philippe Leduc");
MODULE_LICENSE("GPL");
