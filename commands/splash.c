#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fcntl.h>
#include <fb.h>
#include <image_renderer.h>
#include <graphic_utils.h>

static int do_splash(int argc, char *argv[])
{
	int ret, opt, fd;
	char *fbdev = "/dev/fb0";
	void *fb;
	struct fb_info info;
	char *image_file;
	int startx = -1, starty = -1;
	int xres, yres;
	int offscreen = 0;
	u32 bg_color = 0x00000000;
	bool do_bg = false;
	void *offscreenbuf = NULL;

	while((opt = getopt(argc, argv, "f:x:y:ob:")) > 0) {
		switch(opt) {
		case 'f':
			fbdev = optarg;
			break;
		case 'b':
			bg_color = simple_strtoul(optarg, NULL, 0);
			do_bg = true;
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
	image_file = argv[optind];

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

	if (offscreen) {
		int fbsize;
		/* Don't fail if malloc fails, just continue rendering directly
		 * on the framebuffer
		 */

		fbsize = xres * yres * (info.bits_per_pixel >> 3);
		offscreenbuf = malloc(fbsize);
		if (offscreenbuf) {
			if (do_bg)
				memset_pixel(&info, offscreenbuf, bg_color, xres * yres);
			else
				memcpy(offscreenbuf, fb, fbsize);
		}
	} else if (do_bg) {
		memset_pixel(&info, fb, bg_color, xres * yres);
	}

	if (image_renderer_file(&info, image_file, fb, startx, starty,
				offscreenbuf) < 0)
		ret = 1;

	if (offscreenbuf)
		free(offscreenbuf);

	close(fd);

	return ret;

failed_memmap:
	close(fd);

	return 1;
}

BAREBOX_CMD_HELP_START(splash)
BAREBOX_CMD_HELP_USAGE("splash [OPTIONS] FILE\n")
BAREBOX_CMD_HELP_SHORT("Show the bitmap FILE on the framebuffer.\n")
BAREBOX_CMD_HELP_OPT  ("-f <fb>",   "framebuffer device (/dev/fb0)\n")
BAREBOX_CMD_HELP_OPT  ("-x <xofs>", "x offset (default center)\n")
BAREBOX_CMD_HELP_OPT  ("-y <yofs>", "y offset (default center)\n")
BAREBOX_CMD_HELP_OPT  ("-b <color>", "background color in 0xttrrggbb\n")
BAREBOX_CMD_HELP_OPT  ("-o",        "render offscreen\n")
BAREBOX_CMD_HELP_END

/**
 * @page bmp_command

This command displays a graphics in the bitmap (.bmp) format on the
framebuffer. Currently the bmp command supports images with 8 and 24 bit
color depth.

\todo What does the -o (offscreen) option do?

 */

BAREBOX_CMD_START(splash)
	.cmd		= do_splash,
	.usage		= "show a bmp image",
	BAREBOX_CMD_HELP(cmd_splash_help)
BAREBOX_CMD_END
