/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <mach/linux.h>
#include <fb.h>
#include <errno.h>
#include <gui/graphic_utils.h>

#define to_mask(color) GENMASK(color.length - 1, color.offset)

static void sdlfb_enable(struct fb_info *info)
{
	struct sdl_fb_info sdl_info = {
		.screen_base = info->screen_base,
		.xres = info->xres, .yres = info->yres, .bpp = info->bits_per_pixel,
		.rmask = to_mask(info->red),
		.gmask = to_mask(info->green),
		.bmask = to_mask(info->blue),
		.amask = to_mask(info->transp),
	};

	sdl_video_open(&sdl_info);
}

static void sdlfb_disable(struct fb_info *info)
{
	sdl_video_close();
}

static struct fb_ops sdlfb_ops = {
	.fb_enable	= sdlfb_enable,
	.fb_disable	= sdlfb_disable,
};

static int sdlfb_probe(struct device_d *dev)
{
	struct fb_info *fb;
	int ret = -EIO;

	if (!dev->platform_data)
		return -EIO;

	fb = xzalloc(sizeof(*fb));
	fb->modes.modes = fb->mode = dev->platform_data;
	fb->modes.num_modes = 1;
	fb->xres = fb->mode->xres;
	fb->yres = fb->mode->yres;

	fb->bits_per_pixel = 32;
	fb->transp.length = 8;
	fb->red.length = 8;
	fb->green.length = 8;
	fb->blue.length = 8;
	fb->transp.offset = 24;
	fb->red.offset = 16;
	fb->green.offset = 8;
	fb->blue.offset = 0;

	fb->priv = fb;
	fb->fbops = &sdlfb_ops;

	fb->dev.parent = dev;
	fb->screen_base = xzalloc(fb->xres * fb->yres *
				  fb->bits_per_pixel >> 3);

	/* add runtime hardware info */
	dev->priv = fb;

	ret = register_framebuffer(fb);
	if (!ret)
		return 0;

	kfree(fb->screen_base);
	kfree(fb);
	return ret;
}

static void sdlfb_remove(struct device_d *dev)
{
	struct fb_info *fb = dev->priv;

	kfree(fb->screen_base);
	kfree(fb);
}

static struct driver_d sdlfb_driver = {
	.name	= "sdlfb",
	.probe	= sdlfb_probe,
	.remove	= sdlfb_remove,
};
device_platform_driver(sdlfb_driver);
