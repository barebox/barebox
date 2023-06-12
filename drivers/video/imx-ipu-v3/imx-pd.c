// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * i.MX drm driver - parallel display implementation
 *
 * Copyright (C) 2016 Philippe Leduc
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

#define IMX_PD_OUTPUT_PORT	1

struct imx_pd {
	struct device *dev;
	struct display_timings *timings;
	u32 bus_format;
	struct vpl vpl;
};

static int imx_pd_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *data)
{
	struct imx_pd *imx_pd = container_of(vpl, struct imx_pd, vpl);
	struct ipu_di_mode *mode;

	switch (cmd) {
	case IMX_IPU_VPL_DI_MODE:
		mode = data;

		mode->di_clkflags = IPU_DI_CLKMODE_NON_FRACTIONAL;
		mode->bus_format = imx_pd->bus_format;
		return 0;

	case VPL_GET_VIDEOMODES:
		if (imx_pd->timings) {
			struct display_timings *timings = data;

			timings->num_modes   = imx_pd->timings->num_modes;
			timings->native_mode = imx_pd->timings->native_mode;
			timings->modes       = imx_pd->timings->modes;
			timings->edid        = NULL;
			return 0;
		}
		break;
	}

	if (!imx_pd->timings)
		return vpl_ioctl(vpl, IMX_PD_OUTPUT_PORT, cmd, data);

	return 0;
}

static int imx_pd_probe(struct device *dev)
{
	struct device_node *node = dev->of_node;
	struct imx_pd *imx_pd;
	struct device_node *port;
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
		port = of_graph_get_port_by_id(node, IMX_PD_OUTPUT_PORT);
		if (!port) {
			dev_err(dev, "Neither display timings in nor remote panel found in node\n");
			return -EINVAL;
		}
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
MODULE_DEVICE_TABLE(of, imx_pd_dt_ids);

static struct driver imx_pd_driver = {
	.probe			  = imx_pd_probe,
	.of_compatible	= imx_pd_dt_ids,
	.name				= "imx-parallel-display",
};
device_platform_driver(imx_pd_driver);

MODULE_DESCRIPTION("i.MX Parallel display driver");
MODULE_AUTHOR("Philippe Leduc");
MODULE_LICENSE("GPL");
