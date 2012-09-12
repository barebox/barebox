#include <common.h>
#include <fs.h>
#include <errno.h>
#include <malloc.h>
#include <fb.h>
#include <bmp_layout.h>
#include <asm/byteorder.h>
#include <graphic_utils.h>

int bmp_render_file(struct fb_info *info, const char* bmpfile, void* fb,
		    int startx, int starty, int xres, int yres, void* offscreenbuf)
{
	struct bmp_image *bmp;
	int sw, sh, width, height;
	int bits_per_pixel, fbsize;
	int bmpsize;
	int ret = 0;
	void *adr, *buf;
	char *image;

	bmp = read_file(bmpfile, &bmpsize);
	if (!bmp) {
		printf("unable to read %s\n", bmpfile);
		return -ENOMEM;
	}

	if (bmp->header.signature[0] != 'B' ||
	      bmp->header.signature[1] != 'M') {
		printf("No valid bmp file\n");
		ret = -EINVAL;
		goto err;
	}

	sw = le32_to_cpu(bmp->header.width);
	sh = le32_to_cpu(bmp->header.height);

	if (startx < 0) {
		startx = (xres - sw) / 2;
		if (startx < 0)
			startx = 0;
	}

	if (starty < 0) {
		starty = (yres - sh) / 2;
		if (starty < 0)
			starty = 0;
	}

	width = min(sw, xres - startx);
	height = min(sh, yres - starty);

	bits_per_pixel = le16_to_cpu(bmp->header.bit_count);
	fbsize = xres * yres * (info->bits_per_pixel >> 3);

	buf = offscreenbuf ? offscreenbuf : fb;

	if (bits_per_pixel == 8) {
		int x, y;
		struct bmp_color_table_entry *color_table = bmp->color_table;

		for (y = 0; y < height; y++) {
			image = (char *)bmp +
					le32_to_cpu(bmp->header.data_offset);
			image += (sh - y - 1) * sw * (bits_per_pixel >> 3);
			adr = buf + ((y + starty) * xres + startx) *
					(info->bits_per_pixel >> 3);
			for (x = 0; x < width; x++) {
				int pixel;

				pixel = *image;

				set_rgb_pixel(info, adr, color_table[pixel].red,
						color_table[pixel].green,
						color_table[pixel].blue);
				adr += info->bits_per_pixel >> 3;

				image += bits_per_pixel >> 3;
			}
		}
	} else if (bits_per_pixel == 24) {
		int x, y;

		for (y = 0; y < height; y++) {
			image = (char *)bmp +
					le32_to_cpu(bmp->header.data_offset);
			image += (sh - y - 1) * sw * (bits_per_pixel >> 3);
			adr = buf + ((y + starty) * xres + startx) *
					(info->bits_per_pixel >> 3);
			for (x = 0; x < width; x++) {
				char *pixel;

				pixel = image;

				set_rgb_pixel(info, adr, pixel[2], pixel[1],
						pixel[0]);
				adr += info->bits_per_pixel >> 3;

				image += bits_per_pixel >> 3;
			}
		}
	} else
		printf("bmp: illegal bits per pixel value: %d\n", bits_per_pixel);

	if (offscreenbuf)
		memcpy(fb, offscreenbuf, fbsize);

	free(bmp);
	return sh;

err:
	free(bmp);
	return ret;
}
