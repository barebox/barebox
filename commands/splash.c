/*
 * (C) Copyright 2007 Pengutronix
 * Sascha Hauer, <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>

#ifdef CFG_SPLASH

#include <command.h>
#include <bmp_layout.h>
#include <splash.h>
#include <asm/byteorder.h>

static struct fb_data *fbd = NULL;

int splash_set_fb_data(struct fb_data *d)
{
	fbd = d;
	return 0;
}

int do_splash(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	ulong addr;
	bmp_image_t *bmp;
	int fb_line_length, bmp_line_length;
	int copy_width, copy_height;
	int colors;
	int i, x, y;
	char *src, *dst;
	unsigned long *dstl, *srcl;

	if (argc == 1) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (!fbd) {
		printf("No framebuffer found\n");
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);

	bmp = (bmp_image_t *)addr;

	if (!((bmp->header.signature[0]=='B') &&
	      (bmp->header.signature[1]=='M'))) {
		printf("No valid bmp file found on address 0x%08x\n",addr);
		return 1;
	}

	if (fbd->bpp != le16_to_cpu(bmp->header.bit_count)) {
		printf("Display image depth does not match framebuffer image depth\n");
		printf("(display depth: %dbpp image depth: %dbpp\n", fbd->bpp,
				le16_to_cpu(bmp->header.bit_count));
		return 1;
	}

	colors = 1 << le16_to_cpu (bmp->header.bit_count);
	printf("Image size    : %d x %d\n", le32_to_cpu(bmp->header.width),
	       le32_to_cpu(bmp->header.height));
	printf("Bits per pixel: %d\n", le16_to_cpu(bmp->header.bit_count));
	printf("colors used   : %d\n", colors);

	/* Set color map to zero */
	for (i = 0; i < colors; i++)
		fbd->setcolreg(fbd, i, 0, 0, 0);

	fb_line_length = (fbd->xres * fbd->bpp) >> 3;
	bmp_line_length = (le32_to_cpu(bmp->header.width) *
			  le16_to_cpu(bmp->header.bit_count)) >> 3;
	if (bmp_line_length & 0x3)
		bmp_line_length = (bmp_line_length & ~3) + 4;
	copy_width = min(fb_line_length, bmp_line_length);
	copy_height = min(fbd->yres, le32_to_cpu(bmp->header.height));

	memset(fbd->fb, 0, fb_line_length * fbd->yres);

//	printf("copy_width: %d copy_height: %d fb_line_length: %d "
//		"bmp_line_length: %d\n", copy_width, copy_height,
//		fb_line_length, bmp_line_length);
	for (y = 0; y < copy_height; y++) {
		dst = (char*)fbd->fb + y * fb_line_length;
		src = (char*)(addr + bmp->header.data_offset) +
			(le32_to_cpu(bmp->header.height) - y - 1) * bmp_line_length;

//		printf("dst: %d src: %d\n",
//			(unsigned long)dst - (unsigned long)fbd->fb,
//			(unsigned long)src - (unsigned long)(addr + bmp->header.data_offset));

		dstl = (unsigned long *)dst;
		srcl = (unsigned long *)((unsigned long)src & ~3);
		for (x = 0; x < (copy_width >> 2); x++) {
			*dstl++ = *srcl++;
		}
	}

	/* Set color map */
	for (i = 0; i < colors; i++) {
		bmp_color_table_entry_t cte = bmp->color_table[i];
		fbd->setcolreg(fbd, i, cte.red, cte.green, cte.blue);
	}

	return 0;
}

U_BOOT_CMD(
	splash,	2,	0,	do_splash,
	"splash <imageaddr> - display a splash screen.",
	"                     Only color mapped\n"
	"                     bmp files are supported. Images smaller than\n"
	"                     the screen are put in the top left corner, from \n"
	"                     bigger images only the top left corner of the\n"
	"                     image is displayed\n"
);

#endif /* CFG_SPLASH */

