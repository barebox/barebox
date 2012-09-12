#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include <asm/byteorder.h>
#include <init.h>
#include <image_renderer.h>
#include <graphic_utils.h>
#include <linux/zlib.h>

#include "png.h"

z_stream png_stream;
static int initialized;

int png_uncompress_init(void)
{
	if (!initialized++) {
		png_stream.workspace = malloc(zlib_inflate_workspacesize());
		if (!png_stream.workspace) {
			initialized = 0;
			return -ENOMEM;
		}
		png_stream.next_in = NULL;
		png_stream.avail_in = 0;
		zlib_inflateInit(&png_stream);
	}
	return 0;
}

void png_uncompress_exit(void)
{
	if (!--initialized) {
		zlib_inflateEnd(&png_stream);
		vfree(png_stream.workspace);
	}
}

static int png_renderer(struct fb_info *info, struct image *img, void* fb,
		int startx, int starty, void* offscreenbuf)
{
	int width, height;
	void *buf;
	int xres, yres;

	xres = info->xres;
	yres = info->yres;

	if (startx < 0) {
		startx = (xres - img->width) / 2;
		if (startx < 0)
			startx = 0;
	}

	if (starty < 0) {
		starty = (yres - img->height) / 2;
		if (starty < 0)
			starty = 0;
	}

	width = min(img->width, xres - startx);
	height = min(img->height, yres - starty);

	buf = offscreenbuf ? offscreenbuf : fb;

	rgba_blend(info, img->data, buf, height, width, startx, starty, true);

	if (offscreenbuf) {
		int fbsize;

		fbsize = xres * yres * (info->bits_per_pixel >> 3);
		memcpy(fb, offscreenbuf, fbsize);
	}

	return img->height;
}

static struct image_renderer png = {
	.type = filetype_png,
	.open = png_open,
	.close = png_close,
	.renderer = png_renderer,
};

static int png_init(void)
{
	return image_renderer_register(&png);
}
fs_initcall(png_init);
