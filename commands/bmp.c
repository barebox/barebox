#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fcntl.h>
#include <fb.h>
#include <bmp_layout.h>
#include <asm/byteorder.h>

static inline void set_pixel(struct fb_info *info, void *adr, int r, int g, int b)
{
	u32 px;

	px = (r >> (8 - info->red.length)) << info->red.offset |
		(g >> (8 - info->green.length)) << info->green.offset |
		(b >> (8 - info->blue.length)) << info->blue.offset;

	switch (info->bits_per_pixel) {
	case 8:
		break;
	case 16:
		*(u16 *)adr = px;
		break;
	case 32:
		*(u32 *)adr = px;
		break;
	}
}

static int do_bmp(struct command *cmdtp, int argc, char *argv[])
{
	int ret, opt, fd;
	char *fbdev = "/dev/fb0";
	void *fb, *offscreenbuf = NULL;
	struct fb_info info;
	struct bmp_image *bmp;
	char *bmpfile;
	int bmpsize;
	char *image;
	int sw, sh, width, height, startx = -1, starty = -1;
	int bits_per_pixel, fbsize;
	int xres, yres;
	int offscreen = 0;
	void *adr, *buf;

	while((opt = getopt(argc, argv, "f:x:y:o")) > 0) {
		switch(opt) {
		case 'f':
			fbdev = optarg;
			break;
		case 'x':
			startx = simple_strtoul(optarg, NULL, 0);
			break;
		case 'y':
			starty = simple_strtoul(optarg, NULL, 0);
		case 'o':
			offscreen = 1;
		}
	}

	if (optind == argc) {
		printf("no filename given\n");
		return 1;
	}
	bmpfile = argv[optind];

	fd = open(fbdev, O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	fb = memmap(fd, PROT_READ | PROT_WRITE);
	if (fb == (void *)-1) {
		perror("memmap");
		goto failed_memmap;
	}

	ret = ioctl(fd, FBIOGET_SCREENINFO, &info);
	if (ret) {
		perror("ioctl");
		goto failed_memmap;
	}

	xres = info.xres;
	yres = info.yres;

	bmp = read_file(bmpfile, &bmpsize);
	if (!bmp) {
		printf("unable to read %s\n", bmpfile);
		goto failed_memmap;
	}

	if (bmp->header.signature[0] != 'B' ||
	      bmp->header.signature[1] != 'M') {
		printf("No valid bmp file\n");
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
	fbsize = xres * yres * (info.bits_per_pixel >> 3);

	if (offscreen) {
		/* Don't fail if malloc fails, just continue rendering directly
		 * on the framebuffer
		 */
		offscreenbuf = malloc(fbsize);
		if (offscreenbuf)
			memcpy(offscreenbuf, fb, fbsize);
	}

	buf = offscreenbuf ? offscreenbuf : fb;

	if (bits_per_pixel == 8) {
		int x, y;
		struct bmp_color_table_entry *color_table = bmp->color_table;

		for (y = 0; y < height; y++) {
			image = (char *)bmp +
					le32_to_cpu(bmp->header.data_offset);
			image += (sh - y - 1) * sw * (bits_per_pixel >> 3);
			adr = buf + ((y + starty) * xres + startx) *
					(info.bits_per_pixel >> 3);
			for (x = 0; x < width; x++) {
				int pixel;

				pixel = *image;

				set_pixel(&info, adr, color_table[pixel].red,
						color_table[pixel].green,
						color_table[pixel].blue);
				adr += info.bits_per_pixel >> 3;

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
					(info.bits_per_pixel >> 3);
			for (x = 0; x < width; x++) {
				char *pixel;

				pixel = image;

				set_pixel(&info, adr, pixel[2], pixel[1],
						pixel[0]);
				adr += info.bits_per_pixel >> 3;

				image += bits_per_pixel >> 3;
			}
		}
	} else
		printf("bmp: illegal bits per pixel value: %d\n", bits_per_pixel);

	if (offscreenbuf) {
		memcpy(fb, offscreenbuf, fbsize);
		free(offscreenbuf);
	}

	free(bmp);
	close(fd);

	return 0;

failed_memmap:
	close(fd);

	return 1;
}

BAREBOX_CMD_HELP_START(bmp)
BAREBOX_CMD_HELP_USAGE("bmp [OPTIONS] FILE\n")
BAREBOX_CMD_HELP_SHORT("Show the bitmap FILE on the framebuffer.\n")
BAREBOX_CMD_HELP_OPT  ("-f <fb>",   "framebuffer device (/dev/fb0)\n")
BAREBOX_CMD_HELP_OPT  ("-x <xofs>", "x offset (default center)\n")
BAREBOX_CMD_HELP_OPT  ("-y <yofs>", "y offset (default center)\n")
BAREBOX_CMD_HELP_OPT  ("-o",        "render offscreen\n")
BAREBOX_CMD_HELP_END

/**
 * @page bmp_command

This command displays a graphics in the bitmap (.bmp) format on the
framebuffer. Currently the bmp command supports images with 8 and 24 bit
color depth.

\todo What does the -o (offscreen) option do?

 */

BAREBOX_CMD_START(bmp)
	.cmd		= do_bmp,
	.usage		= "show a bmp image",
	BAREBOX_CMD_HELP(cmd_bmp_help)
BAREBOX_CMD_END
