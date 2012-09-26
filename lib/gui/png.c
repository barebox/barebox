#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include <asm/byteorder.h>
#include <init.h>
#include <gui/image_renderer.h>
#include <gui/graphic_utils.h>
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

static int png_renderer(struct screen *sc, struct surface *s, struct image *img)
{
	void *buf;
	int width = s->width;
	int height = s->height;
	int startx = s->x;
	int starty = s->y;

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

	buf = gui_screen_redering_buffer(sc);

	rgba_blend(&sc->info, img, buf, height, width, startx, starty, true);

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
