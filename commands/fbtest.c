#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fb.h>
#include <gui/graphic_utils.h>
#include <gui/2d-primitives.h>
#include <linux/gcd.h>
#include <int_sqrt.h>

static void fbtest_pattern_solid(struct screen *sc, u32 color)
{
	const u8 r = (color >> 16) & 0xff;
	const u8 g = (color >>  8) & 0xff;
	const u8 b = (color >>  0) & 0xff;

	gu_fill_rectangle(sc, 0, 0, -1, -1, r, g, b, 0xff);
}

static void fbtest_pattern_bars(struct screen *sc, u32 unused)
{
	int i;

	const u32 xres = sc->info->xres;

	const u32 colors[] =  {
		0xFFFFFF, 	/* white */
		0xFFFF00,	/* yellow */
		0x00FFFF,	/* cyan */
		0x00FF00,	/* green */
		0xFF00FF,	/* magenta */
		0xFF0000,	/* red */
		0x0000FF,	/* blue */
		0x000000,	/* black */
	};

	for (i = 0; i < ARRAY_SIZE(colors); i++) {
		const u8 r = (colors[i] >> 16) & 0xff;
		const u8 g = (colors[i] >>  8) & 0xff;
		const u8 b = (colors[i] >>  0) & 0xff;
		const int dx = xres / ARRAY_SIZE(colors);

		gu_fill_rectangle(sc,
				  i * dx, -1, (i + 1) * dx - 1, -1,
				  r, g, b, 0xff);
	}
}

static void fbtest_pattern_geometry(struct screen *sc, u32 color)
{
	int i;

	const u8 r = (color >> 16) & 0xff;
	const u8 g = (color >>  8) & 0xff;
	const u8 b = (color >>  0) & 0xff;

	const u32 xres = sc->info->xres;
	const u32 yres = sc->info->yres;

	const u8 xcount = xres / gcd(xres, yres);
	const u8 ycount = yres / gcd(xres, yres);

	const struct {
		int x1, y1, x2, y2;
	} borders[] = {
		{ 0,        0,        xres - 1, 0        },
		{ xres - 1, 0,        xres - 1, yres - 1 },
		{ 0,        yres - 1, xres - 1, yres - 1 },
		{ 0,        0,        0,        yres - 1 },
	};

	const int R1 = min(xres, yres) / 2;
	const int h  = xres * xres + yres * yres;
	const int R2 = (int_sqrt(h) / 2 - R1) * 5 / 12;

	const  struct {
		int x0, y0, radius;
	} circles[] = {
		{ xres / 2,  yres / 2,  R1 - 1 },
		{ R2,        R2,        R2 - 1 },
		{ xres - R2, R2,        R2 - 1 },
		{ xres - R2, yres - R2, R2 - 1 },
		{ R2,        yres - R2, R2 - 1 }
	};

	void *buf = gui_screen_render_buffer(sc);

	gu_memset_pixel(sc->info, buf, ~color,
			sc->s.width * sc->s.height);

	for (i = 0; i < ARRAY_SIZE(borders); i++)
		gu_draw_line(sc,
			     borders[i].x1, borders[i].y1,
			     borders[i].x2, borders[i].y2,
			     r, g, b, 0xff, 10);

	for (i = 0; i < ARRAY_SIZE(circles); i++)
		gu_draw_circle(sc,
			       circles[i].x0, circles[i].y0,
			       circles[i].radius,
			       r, g, b, 0xff);

	for (i = 1; i < ycount; i++) {
		const int y = (yres - 1) * i / ycount;
		gu_draw_line(sc,
			     0, y, xres - 1, y,
			     r, g, b, 0xff, 0);
	}


	for (i = 1; i < xcount; i++) {
		const int x = (xres - 1) * i / xcount;
		gu_draw_line(sc,
			     x, 0, x, yres - 1,
			     r, g, b, 0xff, 0);
	}
}

static void draw_line_r(struct screen *sc, bool rotate_90_ccw,
			int x1, int y1, int x2, int y2,
			uint8_t r, uint8_t g, uint8_t b)
{
	if (rotate_90_ccw)
		gu_draw_line(sc,
			     y1, sc->info->yres - x1,
			     y2, sc->info->yres - x2,
			     r, g, b, 0xff, 0);
	else
		gu_draw_line(sc, x1, y1, x2, y2, r, g, b, 0xff, 0);
}

static void solid_rect_r(struct screen *sc, bool rotate_90_ccw,
			 int x1, int y1, int x2, int y2,
			 uint8_t r, uint8_t g, uint8_t b)
{
	if (rotate_90_ccw)
		gu_fill_rectangle(sc,
				  y1, sc->info->yres - x1,
				  y2, sc->info->yres - x2,
				  r, g, b, 0xff);
	else
		gu_fill_rectangle(sc, x1, y1, x2, y2, r, g, b, 0xff);
}

static void grad_rect_r(struct screen *sc, bool rotate_90_ccw,
			int x1, int y1, int x2, int y2,
			uint8_t r, uint8_t g, uint8_t b)
{
	int x;

	for (x = x1; x <= x2; x++)
		draw_line_r(sc, rotate_90_ccw, x, y1, x, y2,
			    r * (x - x1 + 1) / (x2 - x1 + 1),
			    g * (x - x1 + 1) / (x2 - x1 + 1),
			    b * (x - x1 + 1) / (x2 - x1 + 1));
}

static void anchor_rect_r(struct screen *sc, bool rotate_90_ccw,
			  int x1, int y1, int x2, int y2)
{
	int dx = (x2 - x1 + 1) / 6, dy = (y2 - y1 + 1) / 6;

	solid_rect_r(sc, rotate_90_ccw,
		     x1, y1, x2, y2,
		     0xff, 0xff, 0xff);

	solid_rect_r(sc, rotate_90_ccw,
		     x1 + dx, y1 + dy, x2 - dx, y2 - dy,
		     0, 0, 0);

	solid_rect_r(sc, rotate_90_ccw,
		     x1 + 2 * dx, y1 + 2 * dy, x2 - 2 * dx, y2 - 2 * dy,
		     0xff, 0xff, 0xff);
}


static void fbtest_pattern_gradient(struct screen *sc, u32 unused)
{
	bool rotate_90_ccw;
	int w, h, border;

	if (sc->info->xres > sc->info->yres) {
		w = sc->info->xres;
		h = sc->info->yres;
		rotate_90_ccw = false;
	} else {
		w = sc->info->yres;
		h = sc->info->xres;
		rotate_90_ccw = true;
	}

	solid_rect_r(sc, rotate_90_ccw, 0, 0, w - 1, h - 1, 0, 0, 0);

	border = h / 5;

	anchor_rect_r(sc, rotate_90_ccw,
		      border * 3 / 10, border * 3 / 10,
		      border * 7 / 10, border * 7 / 10);
	anchor_rect_r(sc, rotate_90_ccw,
		      w - border * 7 / 10, border * 3 / 10,
		      w - border * 3 / 10, border * 7 / 10);
	anchor_rect_r(sc, rotate_90_ccw,
		      w - border * 7 / 10, h - border * 7 / 10,
		      w - border * 3 / 10, h - border * 3 / 10);

	grad_rect_r(sc, rotate_90_ccw,
		    border, border,
		    w - border, border + (h - 2 * border) / 4 - 1,
		    0xff, 0, 0);
	grad_rect_r(sc, rotate_90_ccw,
		    border, border + (h - 2 * border) / 4,
		    w - border, border + (h - 2 * border) / 2 - 1,
		    0, 0xff, 0);
	grad_rect_r(sc, rotate_90_ccw,
		    border, border + (h - 2 * border) / 2,
		    w - border, border + (h - 2 * border) * 3 / 4 - 1,
		    0, 0, 0xff);
	grad_rect_r(sc, rotate_90_ccw,
		    border, border + (h - 2 * border) * 3 / 4,
		    w - border, h - border,
		    0xff, 0xff, 0xff);
}

static int do_fbtest(int argc, char *argv[])
{
	struct screen *sc;
	int opt;
	unsigned int i;
	const char *pattern_name = NULL;
	char *fbdev = "/dev/fb0";
	void (*pattern) (struct screen *sc, u32 color) = NULL;
	u32 color = 0xffffff;

	struct {
		const char *name;
		void (*func) (struct screen *sc, u32 color);
	} patterns[] = {
		{ "solid",    fbtest_pattern_solid    },
		{ "geometry", fbtest_pattern_geometry },
		{ "bars",     fbtest_pattern_bars     },
		{ "gradient", fbtest_pattern_gradient },
	};

	while((opt = getopt(argc, argv, "d:p:c:")) > 0) {
		switch(opt) {
		case 'd':
			fbdev = optarg;
			break;
		case 'p':
			pattern_name = optarg;
			break;
		case 'c':
			color = simple_strtoul(optarg, NULL, 16);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (pattern_name) {
		for (i = 0; i < ARRAY_SIZE(patterns); i++)
			if (!strcmp(pattern_name, patterns[i].name))
				pattern = patterns[i].func;

		if (!pattern) {
			printf("Unknown pattern: %s\n", pattern_name);
			return -EINVAL;
		}
	}

	sc = fb_open(fbdev);
	if (IS_ERR(sc)) {
		perror("fd_open");
		return PTR_ERR(sc);
	}

	if (!pattern_name) {
		printf("No pattern selected. Cycling through all of them.\n");
		printf("Press Ctrl-C to stop\n");

		i = 0;
		for (;;) {
			uint64_t start;
			pattern = patterns[i++ % ARRAY_SIZE(patterns)].func;
			pattern(sc, color);
			gu_screen_blit(sc);
			fb_flush(sc->info);

			start = get_time_ns();
			while (!is_timeout(start, 2 * SECOND))
				if (ctrlc())
					goto done;
		}
	} else {
		pattern(sc, color);
		gu_screen_blit(sc);
	}
done:
	fb_close(sc);

	return 0;
}

BAREBOX_CMD_HELP_START(fbtest)
BAREBOX_CMD_HELP_TEXT("This command displays a test pattern on a screen")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-d <fbdev>\t",    "framebuffer device (default /dev/fb0)")
BAREBOX_CMD_HELP_OPT ("-c color\t", "color, in hex RRGGBB format")
BAREBOX_CMD_HELP_OPT ("-p pattern\t", "pattern name (solid, geometry, bars, gradient)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(fbtest)
	.cmd		= do_fbtest,
	BAREBOX_CMD_DESC("display a test pattern")
	BAREBOX_CMD_OPTS("[-dcp]")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_fbtest_help)
BAREBOX_CMD_END
