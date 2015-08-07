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
};

struct fbc_priv {
	struct console_device cdev;
	struct fb_info *fb;

	struct screen *sc;

	struct param_d *par_font;
	int par_font_val;

	int font_width, font_height;
	const u8 *fontdata;
	unsigned int cols, rows;
	unsigned int x, y; /* cursor position */

	enum state_t state;

	int color;
	int bgcolor;

#define ANSI_FLAG_INVERT	(1 << 0)
#define ANSI_FLAG_BRIGHT	(1 << 1)
	unsigned flags;

	int csipos;
	u8 csi[256];

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

	memset(buf, 0, priv->fb->line_length * priv->fb->yres);
	gu_screen_blit(priv->sc);
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

static void drawchar(struct fbc_priv *priv, int x, int y, char c)
{
	void *buf;
	int bpp = priv->fb->bits_per_pixel >> 3;
	void *adr;
	int i;
	const char *inbuf;
	int line_length;
	u32 color, bgcolor;
	struct rgb *rgb;

	buf = gui_screen_render_buffer(priv->sc);

	inbuf = &priv->fontdata[c * priv->font_height];

	line_length = priv->fb->line_length;

	color = priv->flags & ANSI_FLAG_INVERT ? priv->bgcolor : priv->color;
	bgcolor = priv->flags & ANSI_FLAG_INVERT ? priv->color : priv->bgcolor;

	if (priv->flags & ANSI_FLAG_BRIGHT)
		color += 8;

	rgb = &colors[color];
	color = gu_rgb_to_pixel(priv->fb, rgb->r, rgb->g, rgb->b, 0xff);

	rgb = &colors[bgcolor];
	bgcolor = gu_rgb_to_pixel(priv->fb, rgb->r, rgb->g, rgb->b, 0xff);

	for (i = 0; i < priv->font_height; i++) {
		uint8_t t = inbuf[i];
		int j;

		adr = buf + line_length * (y * priv->font_height + i) + x * priv->font_width * bpp;

		for (j = 0; j < priv->font_width; j++) {
			if (t & 0x80)
				gu_set_pixel(priv->fb, adr, color);
			else
				gu_set_pixel(priv->fb, adr, bgcolor);

			adr += priv->fb->bits_per_pixel >> 3;
			t <<= 1;
		}
	}
}

static void video_invertchar(struct fbc_priv *priv, int x, int y)
{
	void *buf;

	buf = gui_screen_render_buffer(priv->sc);

	gu_invert_area(priv->fb, buf, x * priv->font_width, y * priv->font_height,
			priv->font_width, priv->font_height);
	gu_screen_blit_area(priv->sc, x * priv->font_width, y * priv->font_height,
			priv->font_width, priv->font_height);
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
			priv->x = priv->cols;
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

		gu_screen_blit_area(priv->sc, priv->x * priv->font_width,
				priv->y * priv->font_height,
				priv->font_width, priv->font_height);

		priv->x++;
		if (priv->x > priv->cols) {
			priv->y++;
			priv->x = 0;
		}
	}

	if (priv->y > priv->rows) {
		void *buf;
		u32 line_length = priv->fb->line_length;
		int line_height = line_length * priv->font_height;

		buf = gui_screen_render_buffer(priv->sc);

		memcpy(buf, buf + line_height, line_height * (priv->rows + 1));
		memset(buf + line_height * priv->rows, 0, line_height);
		gu_screen_blit(priv->sc);
		priv->y = priv->rows;
	}

	video_invertchar(priv, priv->x, priv->y);

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
	case 'J':
		cls(priv);
		video_invertchar(priv, priv->x, priv->y);
		return;
	case 'H':
		video_invertchar(priv, priv->x, priv->y);
		pos = simple_strtoul(priv->csi, &end, 10);
		priv->y = pos ? pos - 1 : 0;
		pos = simple_strtoul(end + 1, NULL, 10);
		priv->x = pos ? pos - 1 : 0;
		video_invertchar(priv, priv->x, priv->y);
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
			priv->state = LIT;
		}
		break;
	}
	priv->in_console = 0;
}

static int setup_font(struct fbc_priv *priv)
{
	struct fb_info *fb = priv->fb;
	const struct font_desc *font;

	font = find_font_enum(priv->par_font_val);
	if (!font) {
		return -ENOENT;
	}

	priv->font_width = font->width;
	priv->font_height = font->height;
	priv->fontdata = font->data;

	priv->rows = fb->yres / priv->font_height - 1;
	priv->cols = fb->xres / priv->font_width - 1;

	return 0;
}

static int fbc_set_active(struct console_device *cdev, unsigned flags)
{
	struct fbc_priv *priv = container_of(cdev,
					struct fbc_priv, cdev);
	struct fb_info *fb = priv->fb;
	int ret;

	if (priv->active) {
		fb_close(priv->sc);
		priv->active = false;
	}

	if (!(flags & (CONSOLE_STDOUT | CONSOLE_STDERR)))
		return 0;

	ret = setup_font(priv);
	if (ret)
		return ret;

	priv->sc = fb_create_screen(fb);
	if (IS_ERR(priv->sc))
		return PTR_ERR(priv->sc);

	fb_enable(fb);

	priv->state = LIT;

	dev_info(priv->cdev.dev, "framebuffer console %dx%d activated\n",
		priv->cols + 1, priv->rows + 1);

	priv->active = true;

	return 0;
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

int register_fbconsole(struct fb_info *fb)
{
	struct fbc_priv *priv;
	struct console_device *cdev;
	int ret;

	priv = xzalloc(sizeof(*priv));

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
	cdev->devname = "fbconsole";
	cdev->devid = DEVICE_ID_DYNAMIC;
	cdev->set_active = fbc_set_active;

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

	pr_info("registered as %s%d\n", cdev->class_dev.name, cdev->class_dev.id);

	return 0;
}
