// SPDX-License-Identifier: GPL-2.0-or-later OR MIT

#define pr_fmt(fmt) "qoi: " fmt

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include <asm/byteorder.h>
#include <gui/graphic_utils.h>
#include <init.h>
#include <gui/image_renderer.h>
#include <asm/unaligned.h>

#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#include "qoi.h"

static struct image *qoi_open(char *inbuf, int insize)
{
	struct image *img;
	void *data;
	qoi_desc qoi;

	img = calloc(1, sizeof(*img));
	if (!img)
		return ERR_PTR(-ENOMEM);

	data = qoi_decode(inbuf, insize, &qoi, 0);
	if (!data) {
		free(img);
		return ERR_PTR(-ENOMEM);
	}

	img->data = data;
	img->height = qoi.height;
	img->width = qoi.width;
	img->bits_per_pixel = qoi.channels * 8;

	pr_debug("%d x %d  x %d data@0x%p\n", img->width, img->height,
		 img->bits_per_pixel, img->data);
	return img;
}

static void qoi_close(struct image *img)
{
	free(img->data);
}

static int qoi_renderer(struct screen *sc, struct surface *s, struct image *img)
{
	int alpha = img->bits_per_pixel == (4 * 8);
	int width = s->width;
	int height = s->height;
	int startx = s->x;
	int starty = s->y;
	void *buf;

	if (s->width < 0)
		width = img->width;
	if (s->height < 0)
		height = img->height;

	if (startx < 0) {
		startx = (sc->s.width - width) / 2;
		if (startx < 0)
			startx = 0;
	}

	if (starty < 0) {
		starty = (sc->s.height - height) / 2;
		if (starty < 0)
			starty = 0;
	}

	width = min(width, sc->s.width - startx);
	height = min(height, sc->s.height - starty);

	buf = gui_screen_render_buffer(sc);

	gu_rgba_blend(sc->info, img, buf, height, width, startx, starty, alpha);

	return img->height;
}

static struct image_renderer qoi = {
	.type = filetype_qoi,
	.open = qoi_open,
	.close = qoi_close,
	.renderer = qoi_renderer,
};

static int qoi_init(void)
{
	return image_renderer_register(&qoi);
}
fs_initcall(qoi_init);
