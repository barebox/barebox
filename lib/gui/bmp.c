#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include "bmp_layout.h"
#include <asm/byteorder.h>
#include <gui/graphic_utils.h>
#include <init.h>
#include <gui/image_renderer.h>
#include <asm/unaligned.h>

static struct image *bmp_open(char *inbuf, int insize)
{
	struct image *img = calloc(1, sizeof(struct image));
	struct bmp_image *bmp = (struct bmp_image*)inbuf;

	if (!img) {
		free(bmp);
		return ERR_PTR(-ENOMEM);
	}

	img->data = inbuf;
	img->height = get_unaligned_le32(&bmp->header.height);
	img->width = get_unaligned_le32(&bmp->header.width);
	img->bits_per_pixel = get_unaligned_le16(&bmp->header.bit_count);

	pr_debug("bmp: %d x %d  x %d data@0x%p\n", img->width, img->height,
		 img->bits_per_pixel, img->data);

	return img;
}

static void bmp_close(struct image *img)
{
	free(img->data);
}

static int bmp_renderer(struct screen *sc, struct surface *s, struct image *img)
{
	struct bmp_image *bmp = img->data;
	int bits_per_pixel;
	void *adr, *buf;
	char *image;
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

	buf = gui_screen_render_buffer(sc);

	bits_per_pixel = img->bits_per_pixel;

	if (bits_per_pixel == 8) {
		int x, y;
		struct bmp_color_table_entry *color_table = bmp->color_table;

		for (y = 0; y < height; y++) {
			image = (char *)bmp +
					get_unaligned_le32(&bmp->header.data_offset);
			image += (img->height - y - 1) * img->width * (bits_per_pixel >> 3);
			adr = buf + (y + starty) * sc->info->line_length +
					startx * (sc->info->bits_per_pixel >> 3);
			for (x = 0; x < width; x++) {
				int pixel;

				pixel = *image;

				gu_set_rgb_pixel(sc->info, adr, color_table[pixel].red,
						color_table[pixel].green,
						color_table[pixel].blue);
				adr += sc->info->bits_per_pixel >> 3;

				image += bits_per_pixel >> 3;
			}
		}
	} else if (bits_per_pixel == 24) {
		int x, y;

		for (y = 0; y < height; y++) {
			image = (char *)bmp +
					get_unaligned_le32(&bmp->header.data_offset);
			image += (img->height - y - 1) * img->width * (bits_per_pixel >> 3);
			adr = buf + (y + starty) * sc->info->line_length +
					startx * (sc->info->bits_per_pixel >> 3);
			for (x = 0; x < width; x++) {
				char *pixel;

				pixel = image;

				gu_set_rgb_pixel(sc->info, adr, pixel[2], pixel[1],
						pixel[0]);
				adr += sc->info->bits_per_pixel >> 3;

				image += bits_per_pixel >> 3;
			}
		}
	} else
		printf("bmp: illegal bits per pixel value: %d\n", bits_per_pixel);

	return img->height;
}

static struct image_renderer bmp = {
	.type = filetype_bmp,
	.open = bmp_open,
	.close = bmp_close,
	.renderer = bmp_renderer,
	.keep_file_data = 1,
};

static int bmp_init(void)
{
	return image_renderer_register(&bmp);
}
fs_initcall(bmp_init);
