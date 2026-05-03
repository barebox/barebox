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
#include <linux/ctype.h>
#include <charset.h>

enum state_t {
	LIT,				/* Literal input */
	ESC,				/* Start of escape sequence */
	CSI,				/* Reading arguments in "CSI Pn ;...*/
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

enum ansi_color {
	BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
	BRIGHT
};

#define DEFAULT_COLOR	WHITE
#define DEFAULT_BGCOLOR	BLACK

struct fbc_screen_state {
	unsigned int x, y; /* cursor position */

	int color;
	int bgcolor;

	u32 color_rgb;
	u32 bgcolor_rgb;

#define ANSI_FLAG_INVERT	(1 << 0)
#define ANSI_FLAG_BRIGHT	(1 << 1)
#define SGR_ATTRIBUTES		(ANSI_FLAG_INVERT | ANSI_FLAG_BRIGHT)
#define HIDE_CURSOR		(1 << 2)
	unsigned flags;
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

	struct fbc_screen_state cur;

	unsigned int rotation;
	enum state_t state;

	int csipos;
	u8 csi[256];
	unsigned char csi_cmd;

	int active;
	int in_console;

	char utf8_buf[5];	/* UTF-8 stream decoder buffer */
};

static int fbc_getc(struct console_device *cdev)
{
	return 0;
}

static int fbc_tstc(struct console_device *cdev)
{
	return 0;
}

static int fbc_get_size(struct console_device *cdev, int *width, int *height)
{
	struct fbc_priv *priv = container_of(cdev, struct fbc_priv, cdev);

	*width = priv->cols;
	*height = priv->rows;
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
	[BLACK]			= { 0, 0, 0 },
	[RED]			= { 205, 0, 0 },
	[GREEN]			= { 0, 205, 0 },
	[YELLOW]		= { 205, 205, 0 },
	[BLUE]			= { 0, 0, 238 },
	[MAGENTA]		= { 205, 0, 205 },
	[CYAN]			= { 0, 205, 205 },
	[WHITE]			= { 205, 205, 205 },
	[BRIGHT + BLACK]	= { 127, 127, 127 },
	[BRIGHT + RED]		= { 255, 0, 0 },
	[BRIGHT + GREEN]	= { 0, 255, 0 },
	[BRIGHT + YELLOW]	= { 255, 255, 0 },
	[BRIGHT + BLUE]		= { 92, 92, 255 },
	[BRIGHT + MAGENTA]	= { 255, 0, 255 },
	[BRIGHT + CYAN]		= { 0, 255, 255 },
	[BRIGHT + WHITE]	= { 255, 255, 255 },
};

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

static void drawchar(struct fbc_priv *priv, int x, int y, int c)
{
	void *buf;
	int bpp = priv->fb->bits_per_pixel >> 3;
	void *adr;
	int i;
	const uint8_t *inbuf;
	int line_length;
	int xstep;
	int ystep;
	int startx;
	int starty;

	buf = gui_screen_render_buffer(priv->sc);

	i = find_font_index(priv->font, c);
	inbuf = priv->font->data + i;

	line_length = priv->fb->line_length;

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
				gu_set_pixel(priv->fb, adr + j * xstep,
					     priv->cur.color_rgb);
			else
				gu_set_pixel(priv->fb, adr + j * xstep,
					     priv->cur.bgcolor_rgb);

			mask >>= 1;

		}

		adr += ystep;

		inbuf++;
		mask = 0x80;
	}

	fb_blit_area(priv, x, y);
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

static void toggle_cursor_visibility(struct fbc_priv *priv)
{
	if (!(priv->cur.flags & HIDE_CURSOR))
		video_invertchar(priv, priv->cur.x, priv->cur.y);
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
	struct fbc_screen_state *cur = &priv->cur;

	toggle_cursor_visibility(priv);

	switch (c) {
	case '\007': /* bell: ignore */
		break;
	case '\b':
		if (cur->x > 0) {
			cur->x--;
		} else if (cur->y > 0) {
			cur->x = priv->cols - 1;
			cur->y--;
		}
		break;
	case '\n':
	case '\013': /* Vertical tab is the same as Line Feed */
		cur->y++;
		break;

	case '\r':
		cur->x = 0;
		break;

	case '\t':
		cur->x = (cur->x + 8) & ~0x3;
		break;

	default:
		drawchar(priv, priv->cur.x, priv->cur.y, c);

		cur->x++;
		if (cur->x >= priv->cols) {
			cur->y++;
			cur->x = 0;
		}
	}

	if (cur->y >= priv->rows) {
		fb_scroll_up(priv);
		cur->y = priv->rows - 1;
	}

	toggle_cursor_visibility(priv);
}

static void fbc_update_colors(struct fbc_priv *priv, int color, int bgcolor)
{
	struct fbc_screen_state *cur = &priv->cur;
	struct rgb *rgb;

	if (color >= 0)
		cur->color = color;
	if (bgcolor >= 0)
		cur->bgcolor = bgcolor;

	if (cur->flags & ANSI_FLAG_INVERT) {
		color = cur->bgcolor;
		bgcolor = cur->color;
	} else {
		color = cur->color;
		bgcolor = cur->bgcolor;
	}

	if (cur->flags & ANSI_FLAG_BRIGHT)
		color += BRIGHT;

	rgb = &colors[color];
	cur->color_rgb = gu_rgb_to_pixel(priv->fb, rgb->r, rgb->g, rgb->b, 0xff);

	rgb = &colors[bgcolor];
	cur->bgcolor_rgb = gu_rgb_to_pixel(priv->fb, rgb->r, rgb->g, rgb->b, 0x0);
}

static void fbc_reset_colors(struct fbc_priv *priv)
{
	fbc_update_colors(priv, DEFAULT_COLOR, DEFAULT_BGCOLOR);
}

static void fbc_parse_colors(struct fbc_priv *priv)
{
	struct fbc_screen_state *cur = &priv->cur;
	int color = -1, bgcolor = -1;
	int code;
	char *str;

	str = priv->csi;

	while (1) {
		code = simple_strtoul(str, &str, 10);
		switch (code) {
		case 0:
			cur->flags &= ~SGR_ATTRIBUTES;
			color = DEFAULT_COLOR;
			bgcolor = DEFAULT_BGCOLOR;
			break;
		case 1:
			cur->flags |= ANSI_FLAG_BRIGHT;
			break;
		case 7:
			cur->flags |= ANSI_FLAG_INVERT;
			break;
		case 30 ... 37:
			color = code - 30;
			break;
		case 39:
			color = DEFAULT_COLOR;
			break;
		case 40 ... 47:
			bgcolor = code - 40;
			break;
		case 49:
			bgcolor = DEFAULT_BGCOLOR;
			break;
		case 90 ... 97:
			cur->flags |= ANSI_FLAG_BRIGHT;
			color = code - 90;
			break;
		}

		if (*str != ';')
			break;
		str++;
	}

	fbc_update_colors(priv, color, bgcolor);
}

static void fbc_set_cursor_row(struct fbc_priv *priv, int y)
{
	priv->cur.y = clamp_t(int, y, 0, priv->rows - 1);
}

static void fbc_set_cursor_col(struct fbc_priv *priv, unsigned int x)
{
	priv->cur.x = clamp_t(int, x, 0, priv->cols - 1);
}

static bool fbc_parse_csi(struct fbc_priv *priv)
{
	char *end;
	unsigned char last;
	int pos, i;

	last = priv->csi[priv->csipos - 1];

	switch (last) {
	case 'm':
		fbc_parse_colors(priv);
		break;
	case 'h':
		/* suffix for vt100 "[?25h" */
		switch (priv->csi_cmd) {
		case '?': /* cursor visible */
			if (!(priv->cur.flags & HIDE_CURSOR))
				break;

			priv->cur.flags &= ~HIDE_CURSOR;
			/* show cursor now */
			toggle_cursor_visibility(priv);
			return true;
		}
		break;
	case 'l':
		/* suffix for vt100 "[?25l" */
		switch (priv->csi_cmd) {
		case '?': /* cursor invisible */
			/* hide cursor now */
			toggle_cursor_visibility(priv);
			priv->cur.flags |= HIDE_CURSOR;
			return true;
		}
		break;
	case 'J':
		cls(priv);
		toggle_cursor_visibility(priv);
		return true;
	case 'H':
		toggle_cursor_visibility(priv);

		pos = simple_strtoul(priv->csi, &end, 10);
		fbc_set_cursor_row(priv, pos - 1);

		pos = simple_strtoul(end + 1, NULL, 10);
		fbc_set_cursor_col(priv, pos - 1);

		toggle_cursor_visibility(priv);
		return true;
	case 'A' ... 'D': {
		pos = simple_strtoul(priv->csi, &end, 10) ?: 1;
		toggle_cursor_visibility(priv);

		switch (last) {
		case 'A': /* cursor up */
			fbc_set_cursor_row(priv, priv->cur.y - pos);
			break;
		case 'B': /* cursor down */
			fbc_set_cursor_row(priv, priv->cur.y + pos);
			break;
		case 'C': /* cursor forward */
			fbc_set_cursor_col(priv, priv->cur.x + pos);
			break;
		case 'D': /* cursor back */
			fbc_set_cursor_col(priv, priv->cur.x - pos);
			break;
		}

		toggle_cursor_visibility(priv);
		return true;
	}
	case 'K':
		pos = simple_strtoul(priv->csi, &end, 10);
		toggle_cursor_visibility(priv);
		switch (pos) {
		case 0:
			for (i = priv->cur.x; i < priv->cols; i++)
				drawchar(priv, i, priv->cur.y, ' ');
			break;
		case 1:
			for (i = 0; i <= priv->cur.x; i++)
				drawchar(priv, i, priv->cur.y, ' ');
			break;
		}
		toggle_cursor_visibility(priv);
		return true;
	}

	return false;
}

static void fbc_putc(struct console_device *cdev, char c)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);
	struct fb_info *fb = priv->fb;
	bool queue_flush = false;

	if (priv->in_console)
		return;
	priv->in_console = 1;

	switch (priv->state) {
	case LIT:
		switch (c) {
		case '\033':
			priv->utf8_buf[0] = '\0';
			priv->state = ESC;
			break;
		case '\0':
			break;
		default: {
			int cp = c;

			if (IS_ENABLED(CONFIG_FRAMEBUFFER_CONSOLE_UTF8)) {
				/* All our fonts are CP437, so convert it
				 * directly to that code page.
				 */
				cp = utf8_to_cp437_stream(c, priv->utf8_buf);
				if (!cp)
					break;
			}

			printchar(priv, cp);
			queue_flush = true;
			break;
		}
		}
		break;
	case ESC:
		switch (c) {
		case '[':
			priv->state = CSI;
			priv->csipos = 0;
			memset(priv->csi, 0, 6);
			break;
		default:
			priv->state = LIT;
			break;
		}
		break;
	case CSI:
		if (c == '\033') {
			priv->state = ESC;
			priv->csi_cmd = -1;
			break;
		}

		priv->csi[priv->csipos++] = c;
		if (priv->csipos == 255) {
			priv->csipos = 0;
			priv->state = LIT;
			priv->csi_cmd = -1;
			break;
		}

		switch (c) {
		case '?':
			/* DEC private sequences use '?' as a parameter prefix byte.
			 * Record '?' in csi_cmd and continue CSI parameter
			 * accumulation for digits and the final byte that follows.
			 */
			priv->csi_cmd = c;
			break;
		case '0' ... '9':
		case ';':
		case ':':
			break;
		default:
			queue_flush = fbc_parse_csi(priv);
			priv->state = LIT;
			priv->csi_cmd = -1;
		}
		break;
	}
	priv->in_console = 0;

	if (queue_flush)
		fb_flush(fb);
}

static int setup_font(struct fbc_priv *priv)
{
	const struct font_desc *font;
	unsigned int height = priv->fb->yres - priv->margin.top - priv->margin.bottom;
	unsigned int width = priv->fb->xres - priv->margin.left - priv->margin.right;
	unsigned int newrows, newcols;

	font = find_font_enum(priv->par_font_val);
	if (!font) {
		return -ENOENT;
	}

	priv->font = font;

	switch (priv->rotation) {
	case FBCONSOLE_ROTATE_0:
	case FBCONSOLE_ROTATE_180:
		newrows = height / priv->font->height;
		newcols = width / priv->font->width;
		break;
	case FBCONSOLE_ROTATE_90:
	case FBCONSOLE_ROTATE_270:
		newrows = width / priv->font->height;
		newcols = height / priv->font->width;
		break;
	default:
		return -EINVAL;
	}

	if (priv->rows != newrows || priv->cols != newcols) {
		priv->rows = newrows;
		priv->cols = newcols;
		priv->cur.x = priv->cur.y = 0;
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
	priv->cur.x = 0;
	priv->cur.y = 0;
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
	priv->cur.x = 0;
	priv->cur.y = 0;

	fbc_reset_colors(priv);

	cdev = &priv->cdev;
	cdev->dev = &fb->dev;
	cdev->tstc = fbc_tstc;
	cdev->putc = fbc_putc;
	cdev->getc = fbc_getc;
	cdev->get_size = fbc_get_size;
	cdev->devname = basprintf("fbconsole%s", fbname);
	cdev->devid = DEVICE_ID_SINGLE;
	cdev->open = fbc_open;
	cdev->close = fbc_close;

	ret = console_register(cdev);
	if (ret) {
		pr_err("registering failed with %pe\n", ERR_PTR(ret));
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
