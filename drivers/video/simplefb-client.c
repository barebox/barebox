// SPDX-License-Identifier: GPL-2.0-only
/*
 * Simplest possible simple frame-buffer driver, as a platform device
 *
 * Copyright (c) 2013, Stephen Warren
 *
 * Based on q40fb.c, which was:
 * Copyright (C) 2001 Richard Zidlicky <rz@linux-m68k.org>
 *
 * Also based on offb.c, which was:
 * Copyright (C) 1997 Geert Uytterhoeven
 * Copyright (C) 1996 Paul Mackerras
 */

#include <common.h>
#include <fb.h>
#include <io.h>
#include <linux/platform_data/simplefb.h>
#include <driver.h>
#include <of.h>

static struct fb_ops simplefb_ops;

static struct simplefb_format simplefb_formats[] = SIMPLEFB_FORMATS;

struct simplefb_params {
	u32 width;
	u32 height;
	u32 stride;
	struct simplefb_format *format;
};

struct simplefb {
	struct fb_info info;
	struct fb_videomode mode;
};

static int simplefb_parse_dt(struct device *dev,
			     struct simplefb_params *params)
{
	struct device_node *np = dev->of_node;
	int ret;
	const char *format;
	int i;

	ret = of_property_read_u32(np, "width", &params->width);
	if (ret) {
		dev_err(dev, "Can't parse width property\n");
		return ret;
	}

	ret = of_property_read_u32(np, "height", &params->height);
	if (ret) {
		dev_err(dev, "Can't parse height property\n");
		return ret;
	}

	ret = of_property_read_u32(np, "stride", &params->stride);
	if (ret) {
		dev_err(dev, "Can't parse stride property\n");
		return ret;
	}

	ret = of_property_read_string(np, "format", &format);
	if (ret) {
		dev_err(dev, "Can't parse format property\n");
		return ret;
	}
	params->format = NULL;
	for (i = 0; i < ARRAY_SIZE(simplefb_formats); i++) {
		if (strcmp(format, simplefb_formats[i].name))
			continue;
		params->format = &simplefb_formats[i];
		break;
	}
	if (!params->format) {
		dev_err(dev, "Invalid format value\n");
		return -EINVAL;
	}

	return 0;
}

static int simplefb_probe(struct device *dev)
{
	int ret;
	struct simplefb_params params;
	struct simplefb *simplefb;
	struct fb_info *info;
	struct resource *mem;

	ret = -ENODEV;
	if (dev->of_node)
		ret = simplefb_parse_dt(dev, &params);

	if (ret)
		return ret;

	mem = dev_request_mem_resource(dev, 0);
	if (IS_ERR(mem)) {
		dev_err(dev, "No memory resource\n");
		return PTR_ERR(mem);
	}

	simplefb = xzalloc(sizeof(*simplefb));

	simplefb->mode.name = params.format->name;
	simplefb->mode.xres = params.width;
	simplefb->mode.yres = params.height;

	info = &simplefb->info;
	info->mode = &simplefb->mode;
	info->bits_per_pixel = params.format->bits_per_pixel;
	info->red = params.format->red;
	info->green = params.format->green;
	info->blue = params.format->blue;
	info->transp = params.format->transp;

	info->screen_base = (void *)mem->start;
	info->screen_size = resource_size(mem);


	info->fbops = &simplefb_ops;

	info->dev.parent = dev;
	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(dev, "Unable to register simplefb: %d\n", ret);
		return ret;
	}

	dev_info(dev, "size %s registered\n", size_human_readable(info->screen_size));

	return 0;
}

static const struct of_device_id simplefb_of_match[] = {
	{ .compatible = "simple-framebuffer", },
	{ },
};
MODULE_DEVICE_TABLE(of, simplefb_of_match);

static struct driver simplefb_driver = {
	.name = "simple-framebuffer",
	.of_compatible = simplefb_of_match,
	.probe = simplefb_probe,
};
device_platform_driver(simplefb_driver);

MODULE_AUTHOR("Stephen Warren <swarren@wwwdotorg.org>");
MODULE_DESCRIPTION("Simple framebuffer driver");
MODULE_LICENSE("GPL v2");
