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

static void sdlfb_enable(struct fb_info *info)
{
	int ret;

	ret = sdl_open(info->xres, info->yres, info->bits_per_pixel,
		     info->screen_base);
	if (ret)
		return;
	sdl_get_bitfield_rgba(&info->red, &info->green, &info->blue, &info->transp);

	sdl_start_timer();
}

static void sdlfb_disable(struct fb_info *info)
{
	sdl_stop_timer();
	sdl_close();
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
	fb->bits_per_pixel = 4 << 3;
	fb->xres = fb->mode->xres;
	fb->yres = fb->mode->yres;

	fb->priv = fb;
	fb->fbops = &sdlfb_ops;

	fb->dev.parent = dev;
	fb->screen_base = xzalloc(fb->xres * fb->yres *
				  fb->bits_per_pixel >> 3);

	dev_dbg(dev, "red: length = %d, offset = %d\n",
		fb->red.length, fb->red.offset);
	dev_dbg(dev, "green: length = %d, offset = %d\n",
		fb->green.length, fb->green.offset);
	dev_dbg(dev, "blue: length = %d, offset = %d\n",
		fb->blue.length, fb->blue.offset);
	dev_dbg(dev, "transp: length = %d, offset = %d\n",
		fb->transp.length, fb->transp.offset);

	/* add runtime hardware info */
	dev->priv = fb;

	ret = register_framebuffer(fb);
	if (!ret)
		return 0;

	kfree(fb->screen_base);
	kfree(fb);
	sdl_close();
	return ret;
}

static void sdlfb_remove(struct device_d *dev)
{
	struct fb_info *fb = dev->priv;

	kfree(fb->screen_base);
	kfree(fb);
	sdl_close();
}

static struct driver_d sdlfb_driver = {
	.name	= "sdlfb",
	.probe	= sdlfb_probe,
	.remove	= sdlfb_remove,
};
device_platform_driver(sdlfb_driver);
