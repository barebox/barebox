// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt)     "fbconsole: " fmt

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fb.h>
#include <gui/image_renderer.h>
#include <gui/graphic_utils.h>
#include <linux/font.h>

enum state_t {
	LIT,				/* Literal input */
	ESC,				/* Start of escape sequence */
	CSI,				/* Reading arguments in "CSI Pn ;...*/
	CSI_CNT,
};

enum fbconsole_rotation {
	FBCONSOLE_ROTATE_0,
	FBCONSOLE_ROTATE_90,
	FBCONSOLE_ROTATE_180,
	FBCONSOLE_ROTATE_270,
};

static const char * const rotation_names[] = {
	[FBCONSOLE_ROTATE_0] = "0",
	[FBCONSOLE_ROTATE_90] = "90",
	[FBCONSOLE_ROTATE_180] = "180",
	[FBCONSOLE_ROTATE_270] = "270",
};

struct fbc_priv {
	struct console_device cdev;
	struct fb_info *fb;
	struct {
		u32 top;
		u32 left;
		u32 bottom;
		u32 right;
	} margin;

	struct screen *sc;

	struct param_d *par_font;
	int par_font_val;

	const struct font_desc *font;

	unsigned int cols, rows;
	unsigned int x, y; /* cursor position */

	unsigned int rotation;
	enum state_t state;

	int color;
	int bgcolor;

#define ANSI_FLAG_INVERT	(1 << 0)
#define ANSI_FLAG_BRIGHT	(1 << 1)
#define HIDE_CURSOR		(1 << 2)
	unsigned flags;

	int csipos;
	u8 csi[256];
	unsigned char csi_cmd;

	int active;
	int in_console;
};

static int fbc_getc(struct console_device *cdev)
{
	return 0;
}

static int fbc_tstc(struct console_device *cdev)
{
	return 0;
}

static void cls(struct fbc_priv *priv)
{
	void *buf = gui_screen_render_buffer(priv->sc);
	struct fb_info *fb = priv->fb;
	int width = fb->xres - priv->margin.left - priv->margin.right;
	int height = fb->yres - priv->margin.top - priv->margin.bottom;
	void *adr;

	adr = buf + priv->fb->line_length * priv->margin.top;

	if (!priv->margin.left && !priv->margin.right) {
		memset(adr, 0, priv->fb->line_length * height);
	} else {
		int bpp = priv->fb->bits_per_pixel >> 3;
		int y;

		for (y = 0; y < height; y++) {
			memset(adr + priv->margin.left * bpp, 0, width * bpp);
			adr += priv->fb->line_length;
		}
	}
	gu_screen_blit_area(priv->sc, priv->margin.left, priv->margin.top,
			    width, height);
}

struct rgb {
	u8 r, g, b;
};

static struct rgb colors[] = {
	{ 0, 0, 0 },
	{ 205, 0, 0 },
	{ 0, 205, 0 },
	{ 205, 205, 0 },
	{ 0, 0, 238 },
	{ 205, 0, 205 },
	{ 0, 205, 205 },
	{ 229, 229, 229 },
	{ 127, 127, 127 },
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 255, 255, 0 },
	{ 92, 92, 255 },
	{ 255, 0, 255 },
	{ 0, 255, 255 },
	{ 255, 255, 255 },
};

static void drawchar(struct fbc_priv *priv, int x, int y, int c)
{
	void *buf;
	int bpp = priv->fb->bits_per_pixel >> 3;
	void *adr;
	int i;
	const uint8_t *inbuf;
	int line_length;
	u32 color, bgcolor;
	struct rgb *rgb;
	int xstep;
	int ystep;
	int startx;
	int starty;

	buf = gui_screen_render_buffer(priv->sc);

	i = find_font_index(priv->font, c);
	inbuf = priv->font->data + i;

	line_length = priv->fb->line_length;

	color = priv->flags & ANSI_FLAG_INVERT ? priv->bgcolor : priv->color;
	bgcolor = priv->flags & ANSI_FLAG_INVERT ? priv->color : priv->bgcolor;

	if (priv->flags & ANSI_FLAG_BRIGHT)
		color += 8;

	rgb = &colors[color];
	color = gu_rgb_to_pixel(priv->fb, rgb->r, rgb->g, rgb->b, 0xff);

	rgb = &colors[bgcolor];
	bgcolor = gu_rgb_to_pixel(priv->fb, rgb->r, rgb->g, rgb->b, 0x0);

	adr = buf;

	switch (priv->rotation) {
	case FBCONSOLE_ROTATE_0:
		xstep = bpp;
		ystep = line_length;
		startx = x * priv->font->width;
		starty = y * priv->font->height;
		break;
	case FBCONSOLE_ROTATE_90:
		xstep = line_length;
		ystep = -bpp;
		startx = (priv->rows - y) * priv->font->height - 1;
		starty = x * priv->font->width;
		break;
	case FBCONSOLE_ROTATE_180:
		xstep = -bpp;
		ystep = -line_length;
		startx = (priv->cols - x) * priv->font->width - 1;
		starty = (priv->rows - y) * priv->font->height - 1;
		break;
	case FBCONSOLE_ROTATE_270:
		xstep = -line_length;
		ystep = bpp;
		startx = priv->font->height * y;
		starty = (priv->cols - x) * priv->font->width - 1;
		break;
	default:
		return;
	}

	adr += (priv->margin.left + startx) * bpp;
	adr += (priv->margin.top + starty) * line_length;

	for (i = 0; i < priv->font->height; i++) {
		uint8_t mask = 0x80;
		int j;

		for (j = 0; j < priv->font->width; j++) {
			if (!mask) {
				inbuf++;
				mask = 0x80;
			}

			if (*inbuf & mask)
				gu_set_pixel(priv->fb, adr + j * xstep, color);
			else
				gu_set_pixel(priv->fb, adr + j * xstep, bgcolor);

			mask >>= 1;

		}

		adr += ystep;

		inbuf++;
		mask = 0x80;
	}
}

static void fb_blit_area(struct fbc_priv *priv, int x, int y)
{
	int startx, starty, width, height, fw, fh;
	int mx, my;

	mx = priv->margin.left;
	my = priv->margin.top;

	fw = priv->font->width;
	fh = priv->font->height;

	switch (priv->rotation) {
	case FBCONSOLE_ROTATE_0:
		startx = mx + x * fw;
		starty = my + y * fh;
		width = fw;
		height = fh;
		break;
	case FBCONSOLE_ROTATE_90:
		startx = mx + (priv->rows - y - 1) * fh;
		starty = my + x * fw;
		width = fh;
		height = fw;
		break;
	case FBCONSOLE_ROTATE_180:
		startx = mx + (priv->cols - x - 1) * fw;
		starty = my + (priv->rows - y - 1) * fh;
		width = fw;
		height = fh;
		break;
	case FBCONSOLE_ROTATE_270:
		startx = mx + y * fh;
		starty = my + (priv->cols - x - 1) * fw;
		width = fh;
		height = fw;
		break;
	default:
		return;
	}

	gu_screen_blit_area(priv->sc, startx, starty, width, height);
}

static void video_invertchar(struct fbc_priv *priv, int x, int y)
{
	int startx, starty, width, height, fw, fh;
	void *buf;

	buf = gui_screen_render_buffer(priv->sc);
	buf += priv->margin.top * priv->fb->line_length;
	buf += priv->margin.left * (priv->fb->bits_per_pixel >> 3);

	fw = priv->font->width;
	fh = priv->font->height;

	switch (priv->rotation) {
	case FBCONSOLE_ROTATE_0:
		startx = x * fw;
		starty = y * fh;
		width = fw;
		height = fh;
		break;
	case FBCONSOLE_ROTATE_90:
		startx = (priv->rows - y - 1) * fh;
		starty = x * fw;
		width = fh;
		height = fw;
		break;
	case FBCONSOLE_ROTATE_180:
		startx = (priv->cols - x - 1) * fw;
		starty = (priv->rows - y - 1) * fh;
		width = fw;
		height = fh;
		break;
	case FBCONSOLE_ROTATE_270:
		startx = y * fh;
		starty = (priv->cols - x - 1) * fw;
		width = fh;
		height = fw;
		break;
	default:
		return;
	}

	gu_invert_area(priv->fb, buf, startx, starty, width, height);
	fb_blit_area(priv, x, y);
}

static void show_cursor(struct fbc_priv *priv, int x, int y)
{
	if (!(priv->flags & HIDE_CURSOR))
		video_invertchar(priv, x, y);
}

static void fb_scroll_up_0(struct fbc_priv *priv, void *adr, int width, int height)
{
	u32 line_length = priv->fb->line_length;
	int line_height = line_length * priv->font->height;

	if (!priv->margin.left && !priv->margin.right) {
		memcpy(adr, adr + line_height, line_height * (priv->rows - 1));
		memset(adr + line_height * (priv->rows - 1), 0, line_height);
	} else {
		int bpp = priv->fb->bits_per_pixel >> 3;
		int y;

		for (y = 0; y < height - priv->font->height; y++) {
			memcpy(adr, adr + line_height, width * bpp);
			adr += line_length;
		}

		for (y = height - priv->font->height; y < height; y++) {
			memset(adr, 0, width * bpp);
			adr += line_length;
		}
	}
}

static void fb_scroll_up_180(struct fbc_priv *priv, void *adr, int width, int height)
{
	u32 line_length = priv->fb->line_length;
	int line_height = line_length * priv->font->height;

	if (!priv->margin.left && !priv->margin.right) {
		memmove(adr + line_height, adr, line_height * (priv->rows - 1));
		memset(adr, 0, line_height);
	} else {
		int bpp = priv->fb->bits_per_pixel >> 3;
		int y;

		adr += (height - 1) * line_length;

		for (y = height; y > priv->font->height; y--) {
			memcpy(adr, adr - line_height, width * bpp);
			adr -= line_length;
		}

		for (y = 0; y < priv->font->height; y++) {
			memset(adr, 0, width * bpp);
			adr -= line_length;
		}
	}
}

static void fb_scroll_up_90(struct fbc_priv *priv, void *adr, int width, int height)
{
	u32 line_length = priv->fb->line_length;
	int bpp = priv->fb->bits_per_pixel >> 3;
	int y;

	for (y = 0; y < priv->cols * priv->font->width; y++) {
		memmove(adr + priv->font->height * bpp,
			adr,
			(priv->rows - 1) * priv->font->height * bpp);
		memset(adr, 0, priv->font->height * bpp);
		adr += line_length;
	}
}

static void fb_scroll_up_270(struct fbc_priv *priv, void *adr, int width, int height)
{
	u32 line_length = priv->fb->line_length;
	int bpp = priv->fb->bits_per_pixel >> 3;
	int y;

	for (y = 0; y < priv->cols * priv->font->width; y++) {
		memmove(adr,
			adr + priv->font->height * bpp,
			(priv->rows - 1) * priv->font->height * bpp);
		memset(adr + priv->font->height * bpp * (priv->rows - 1),
		       0x0,
		       priv->font->height * bpp);
		adr += line_length;
	}
}

static void fb_scroll_up(struct fbc_priv *priv)
{
	int width = priv->fb->xres - priv->margin.left - priv->margin.right;
	int height = priv->fb->yres - priv->margin.top - priv->margin.bottom;
	int bpp = priv->fb->bits_per_pixel >> 3;
	void *adr;

	adr = gui_screen_render_buffer(priv->sc);
	adr += priv->margin.top * priv->fb->line_length;
	adr += priv->margin.left * bpp;

	switch (priv->rotation) {
	case FBCONSOLE_ROTATE_0:
		fb_scroll_up_0(priv, adr, width, height);
		break;
	case FBCONSOLE_ROTATE_90:
		fb_scroll_up_90(priv, adr, width, height);
		break;
	case FBCONSOLE_ROTATE_180:
		fb_scroll_up_180(priv, adr, width, height);
		break;
	case FBCONSOLE_ROTATE_270:
		fb_scroll_up_270(priv, adr, width, height);
		break;
	}

	gu_screen_blit_area(priv->sc, priv->margin.left, priv->margin.top,
			    width, height);
}

static void printchar(struct fbc_priv *priv, int c)
{
	video_invertchar(priv, priv->x, priv->y);

	switch (c) {
	case '\007': /* bell: ignore */
		break;
	case '\b':
		if (priv->x > 0) {
			priv->x--;
		} else if (priv->y > 0) {
			priv->x = priv->cols - 1;
			priv->y--;
		}
		break;
	case '\n':
	case '\013': /* Vertical tab is the same as Line Feed */
		priv->y++;
		break;

	case '\r':
		priv->x = 0;
		break;

	case '\t':
		priv->x = (priv->x + 8) & ~0x3;
		break;

	default:
		drawchar(priv, priv->x, priv->y, c);
		fb_blit_area(priv, priv->x, priv->y);

		priv->x++;
		if (priv->x >= priv->cols) {
			priv->y++;
			priv->x = 0;
		}
	}

	if (priv->y >= priv->rows) {
		fb_scroll_up(priv);
		priv->y = priv->rows - 1;
	}

	show_cursor(priv, priv->x, priv->y);

	return;
}

static void fbc_parse_colors(struct fbc_priv *priv)
{
	int code;
	char *str;

	str = priv->csi;

	while (1) {
		code = simple_strtoul(str, &str, 10);
		switch (code) {
		case 0:
			priv->flags = 0;
			priv->color = 8;
			priv->bgcolor = 0;
			break;
		case 1:
			priv->flags |= ANSI_FLAG_BRIGHT;
			break;
		case 7:
			priv->flags |= ANSI_FLAG_INVERT;
			break;
		case 30 ... 37:
			priv->color = code - 30;
			break;
		case 39:
			priv->color = 7;
			break;
		case 40 ... 47:
			priv->bgcolor = code - 40;
			break;
		case 49:
			priv->bgcolor = 0;
			break;
		}

		if (*str != ';')
			break;
		str++;
	}
}

static void fbc_parse_csi(struct fbc_priv *priv)
{
	char *end;
	unsigned char last;
	int pos, i;

	last = priv->csi[priv->csipos - 1];

	switch (last) {
	case 'm':
		fbc_parse_colors(priv);
		return;
	case '?': /* vt100: show/hide cursor */
		priv->csi_cmd = last;
		priv->state = CSI_CNT;
		return;
	case 'h':
		/* suffix for vt100 "[?25h" */
		switch (priv->csi_cmd) {
		case '?': /* cursor visible */
			priv->csi_cmd = -1;
			if (!(priv->flags & HIDE_CURSOR))
				break;

			priv->flags &= ~HIDE_CURSOR;
			/* show cursor now */
			show_cursor(priv, priv->x, priv->y);
			break;
		}
		break;
	case 'l':
		/* suffix for vt100 "[?25l" */
		switch (priv->csi_cmd) {
		case '?': /* cursor invisible */
			priv->csi_cmd = -1;

			/* hide cursor now */
			video_invertchar(priv, priv->x, priv->y);
			priv->flags |= HIDE_CURSOR;

			break;
		}
		break;
	case 'J':
		cls(priv);
		show_cursor(priv, priv->x, priv->y);
		return;
	case 'H':
		show_cursor(priv, priv->x, priv->y);

		pos = simple_strtoul(priv->csi, &end, 10);
		priv->y = clamp(pos - 1, 0, (int) priv->rows - 1);

		pos = simple_strtoul(end + 1, NULL, 10);
		priv->x = clamp(pos - 1, 0, (int) priv->cols - 1);

		show_cursor(priv, priv->x, priv->y);
	case 'K':
		pos = simple_strtoul(priv->csi, &end, 10);
		video_invertchar(priv, priv->x, priv->y);
		switch (pos) {
		case 0:
			for (i = priv->x; i < priv->cols; i++)
				drawchar(priv, i, priv->y, ' ');
			break;
		case 1:
			for (i = 0; i <= priv->x; i++)
				drawchar(priv, i, priv->y, ' ');
			break;
		}
		video_invertchar(priv, priv->x, priv->y);

		break;
	}
}

static void fbc_putc(struct console_device *cdev, char c)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);
	struct fb_info *fb = priv->fb;

	if (priv->in_console)
		return;
	priv->in_console = 1;

	switch (priv->state) {
	case LIT:
		switch (c) {
		case '\033':
			priv->state = ESC;
			break;
		default:
			printchar(priv, c);
		}
		break;
	case ESC:
		switch (c) {
		case '[':
			priv->state = CSI;
			priv->csipos = 0;
			memset(priv->csi, 0, 6);
			break;
		}
		break;
	case CSI:
		priv->csi[priv->csipos++] = c;
		if (priv->csipos == 255) {
			priv->csipos = 0;
			priv->state = LIT;
			return;
		}

		switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case ';':
		case ':':
			break;
		default:
			fbc_parse_csi(priv);
			if (priv->state != CSI_CNT)
				priv->state = LIT;
		}
		break;
	case CSI_CNT:
		priv->state = CSI;
		break;

	}
	priv->in_console = 0;

	fb_flush(fb);
}

static int setup_font(struct fbc_priv *priv)
{
	const struct font_desc *font;
	unsigned int height = priv->fb->yres - priv->margin.top - priv->margin.bottom;
	unsigned int width = priv->fb->xres - priv->margin.left - priv->margin.right;

	font = find_font_enum(priv->par_font_val);
	if (!font) {
		return -ENOENT;
	}

	priv->font = font;

	switch (priv->rotation) {
	case FBCONSOLE_ROTATE_0:
	case FBCONSOLE_ROTATE_180:
		priv->rows = height / priv->font->height;
		priv->cols = width / priv->font->width;
		break;
	case FBCONSOLE_ROTATE_90:
	case FBCONSOLE_ROTATE_270:
		priv->rows = width / priv->font->height;
		priv->cols = height / priv->font->width;
		break;
	}

	return 0;
}

static int fbc_open(struct console_device *cdev)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);
	struct fb_info *fb = priv->fb;
	int ret;

	ret = setup_font(priv);
	if (ret)
		return ret;

	priv->sc = fb_create_screen(fb);
	if (IS_ERR(priv->sc))
		return PTR_ERR(priv->sc);

	fb_enable(fb);

	priv->state = LIT;

	dev_info(priv->cdev.dev, "framebuffer console %dx%d activated\n",
		priv->cols, priv->rows);

	priv->active = true;

	return 0;
}

static int fbc_close(struct console_device *cdev)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);

	if (priv->active) {
		fb_close(priv->sc);
		priv->active = false;

		return 0;
	}

	return -EINVAL;
}

static int set_font(struct param_d *p, void *vpriv)
{
	struct fbc_priv *priv = vpriv;
	struct console_device *cdev = &priv->cdev;

	if (cdev->f_active & (CONSOLE_STDOUT | CONSOLE_STDERR)) {
		cls(priv);
		setup_font(priv);
	}

	return 0;
}

static int set_margin(struct param_d *p, void *vpriv)
{
	struct fbc_priv *priv = vpriv;
	struct console_device *cdev = &priv->cdev;
	int ret;

	if (!priv->font) {
		ret = setup_font(priv);
		if (ret)
			return ret;
	}

	priv->margin.left = min(priv->margin.left,
			priv->fb->xres - priv->margin.right - priv->font->width);
	priv->margin.top = min(priv->margin.top,
			priv->fb->yres - priv->margin.bottom - priv->font->height);
	priv->margin.right = min(priv->margin.right,
			priv->fb->xres - priv->margin.left - priv->font->width);
	priv->margin.bottom = min(priv->margin.bottom,
			priv->fb->yres - priv->margin.top - priv->font->height);

	if (cdev->f_active & (CONSOLE_STDOUT | CONSOLE_STDERR)) {
		cls(priv);
		setup_font(priv);
	}

	return 0;
}

static int set_rotation(struct param_d *p, void *vpriv)
{
	struct fbc_priv *priv = vpriv;

	cls(priv);
	priv->x = 0;
	priv->y = 0;
	setup_font(priv);

	return 0;
}

int register_fbconsole(struct fb_info *fb)
{
	struct fbc_priv *priv;
	struct console_device *cdev;
	int ret;
	const char *fbname;

	priv = xzalloc(sizeof(*priv));

	/*
	 * fbconsoles are usually just numbered and happen to use the same
	 * number as the fb they are registered on, so we have a nice match.
	 * With overlays we can also have fb names like fb0_0 (for the first
	 * overlay on fb0), and we want to have this name in the corresponding
	 * fbconsole. Skip the "fb" though to keep the existing fbconsole name
	 * for the non overlay framebuffers for compatibility.
	 */
	fbname = dev_name(&fb->dev);
	if (strncmp(fbname, "fb", 2))
		pr_notice("unexpected fb name %s\n", fbname);
	else
		fbname += 2;

	priv->fb = fb;
	priv->x = 0;
	priv->y = 0;
	priv->color = 7;
	priv->bgcolor = 0;

	cdev = &priv->cdev;
	cdev->dev = &fb->dev;
	cdev->tstc = fbc_tstc;
	cdev->putc = fbc_putc;
	cdev->getc = fbc_getc;
	cdev->devname = basprintf("fbconsole%s", fbname);
	cdev->devid = DEVICE_ID_SINGLE;
	cdev->open = fbc_open;
	cdev->close = fbc_close;

	ret = console_register(cdev);
	if (ret) {
		pr_err("registering failed with %s\n", strerror(-ret));
		kfree(priv);
		return ret;
	}

	priv->par_font_val = 0;
	priv->par_font = add_param_font(&cdev->class_dev,
			set_font, NULL,
			&priv->par_font_val, priv);

	dev_add_param_uint32(&cdev->class_dev, "margin.top", set_margin,
			NULL, &priv->margin.top, "%u", priv);
	dev_add_param_uint32(&cdev->class_dev, "margin.left", set_margin,
			NULL, &priv->margin.left, "%u", priv);
	dev_add_param_uint32(&cdev->class_dev, "margin.bottom", set_margin,
			NULL, &priv->margin.bottom, "%u", priv);
	dev_add_param_uint32(&cdev->class_dev, "margin.right", set_margin,
			NULL, &priv->margin.right, "%u", priv);
	dev_add_param_enum(&cdev->class_dev, "rotation", set_rotation, NULL,
                           &priv->rotation, rotation_names,
                           ARRAY_SIZE(rotation_names), priv);

	pr_info("registered as %s%d\n", cdev->class_dev.name, cdev->class_dev.id);

	return 0;
}
