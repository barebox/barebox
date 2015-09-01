#include <common.h>
#include <fb.h>
#include <gui/graphic_utils.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <fs.h>
#include <malloc.h>

/**
 * gu_get_pixel_rgb - convert a rgb triplet color to fb format
 * @info: The framebuffer info to convert the pixel for
 * @r: red component
 * @g: green component
 * @b: blue component
 * @t: transparent component
 *
 * This converts a color to the format suitable for writing directly into
 * the framebuffer.
 *
 * Return: The pixel suitable for the framebuffer
 */
u32 gu_rgb_to_pixel(struct fb_info *info, u8 r, u8 g, u8 b, u8 t)
{
	u32 px;

	px = (t >> (8 - info->transp.length)) << info->transp.offset |
		 (r >> (8 - info->red.length)) << info->red.offset |
		 (g >> (8 - info->green.length)) << info->green.offset |
		 (b >> (8 - info->blue.length)) << info->blue.offset;

	return px;
}

/**
 * gu_hex_to_pixel - convert a 32bit color to fb format
 * @info: The framebuffer info to convert the pixel for
 * @color: The color in 0xttrrggbb format
 *
 * This converts a color in 0xttrrggbb format to the format
 * suitable for writing directly into the framebuffer.
 *
 * Return: The pixel suitable for the framebuffer
 */
u32 gu_hex_to_pixel(struct fb_info *info, u32 color)
{
	u32 px;
	u8 t = (color >> 24) & 0xff;
	u8 r = (color >> 16) & 0xff;
	u8 g = (color >> 8 ) & 0xff;
	u8 b = (color >> 0 ) & 0xff;

	if (info->grayscale) {
		 px = (r | g | b) ? 0xffffffff : 0x0;
		 return px;
	}

	return gu_rgb_to_pixel(info, r, g, b, t);
}

static void memsetw(void *s, u16 c, size_t n)
{
	size_t i;
	u16* tmp = s;

	for (i = 0; i < n; i++)
		*tmp++ = c;
}

static void memsetl(void *s, u32 c, size_t n)
{
	size_t i;
	u32* tmp = s;

	for (i = 0; i < n; i++)
		*tmp++ = c;
}

void gu_memset_pixel(struct fb_info *info, void* buf, u32 color, size_t size)
{
	u32 px;
	u8 *screen = buf;

	px = gu_hex_to_pixel(info, color);

	switch (info->bits_per_pixel) {
	case 8:
		memset(screen, (uint8_t)px, size);
		break;
	case 16:
		memsetw(screen, (uint16_t)px, size);
		break;
	case 32:
	case 24:
		memsetl(screen, px, size);
		break;
	}
}

static void get_rgb_pixel(struct fb_info *info, void *adr, u8 *r ,u8 *g, u8 *b)
{
	u32 px;
	u32 rmask, gmask, bmask;

	switch (info->bits_per_pixel) {
	case 16:
		px = *(u16 *)adr;
		break;
	case 32:
		px = *(u32 *)adr;
		break;
	case 8:
	default:
		return;
	}

	rmask = (0xff >> (8 - info->blue.length)) << info->blue.offset |
		(0xff >> (8 - info->green.length)) << info->green.offset;

	gmask = (0xff >> (8 - info->red.length)) << info->red.offset |
		(0xff >> (8 - info->blue.length)) << info->blue.offset;

	bmask = (0xff >> (8 - info->red.length)) << info->red.offset |
		(0xff >> (8 - info->green.length)) << info->green.offset;

	*r = ((px & ~rmask) >> info->red.offset) << (8 - info->red.length);
	*g = ((px & ~gmask) >> info->green.offset) << (8 - info->green.length);
	*b = ((px & ~bmask) >> info->blue.offset) << (8 - info->blue.length);
}

void gu_set_pixel(struct fb_info *info, void *adr, u32 px)
{
	switch (info->bits_per_pixel) {
	case 8:
		break;
	case 16:
		*(u16 *)adr = px & 0xffff;
		break;
	case 32:
		*(u32 *)adr = px;
		break;
	}
}

void gu_set_rgb_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b)
{
	u32 px;

	px = (r >> (8 - info->red.length)) << info->red.offset |
		(g >> (8 - info->green.length)) << info->green.offset |
		(b >> (8 - info->blue.length)) << info->blue.offset;

	gu_set_pixel(info, adr, px);
}

static u8 alpha_mux(int s, int d, int a)
{
	return (d * a + s * (255 - a)) >> 8;
}

void gu_invert_area(struct fb_info *info, void *buf, int startx, int starty, int width,
		int height)
{
	unsigned char *adr;
	int x, y;
	int line_length;
	int bpp = info->bits_per_pixel >> 3;

	line_length = info->line_length;

	for (y = starty; y < starty + height; y++) {
		adr = buf + line_length * y + startx * bpp;

		for (x = 0; x < width * bpp; x++) {
			*adr++ ^= 0xff;
		}
	}
}

void gu_set_rgba_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b, u8 a)
{
	u32 px = 0x0;

	if (!a)
		return;

	if (a != 0xff) {
		if (info->transp.length) {
			px |= (a >> (8 - info->transp.length)) << info->transp.offset;
		} else {
			u8 sr = 0;
			u8 sg = 0;
			u8 sb = 0;

			get_rgb_pixel(info, adr, &sr, &sg, &sb);

			r = alpha_mux(sr, r, a);
			g = alpha_mux(sg, g, a);
			b = alpha_mux(sb, b, a);

			gu_set_rgb_pixel(info, adr, r, g, b);

			return;
		}
	}

	px |= (r >> (8 - info->red.length)) << info->red.offset |
		(g >> (8 - info->green.length)) << info->green.offset |
		(b >> (8 - info->blue.length)) << info->blue.offset;

	gu_set_pixel(info, adr, px);
}

void gu_rgba_blend(struct fb_info *info, struct image *img, void* buf, int height,
	int width, int startx, int starty, bool is_rgba)
{
	unsigned char *adr;
	int x, y;
	int line_length;
	int img_byte_per_pixel = 3;
	void *image;

	if (is_rgba)
		img_byte_per_pixel++;

	line_length = info->line_length;

	for (y = 0; y < height; y++) {
		adr = buf + (y + starty) * line_length +
				startx * (info->bits_per_pixel >> 3);
		image = img->data + (y * img->width *img_byte_per_pixel);

		for (x = 0; x < width; x++) {
			uint8_t *pixel = image;

			if (is_rgba)
				gu_set_rgba_pixel(info, adr, pixel[0], pixel[1],
						pixel[2], pixel[3]);
			else
				gu_set_rgb_pixel(info, adr, pixel[0], pixel[1],
						pixel[2]);
			adr += info->bits_per_pixel >> 3;
			image += img_byte_per_pixel;
		}
	}
}

struct screen *fb_create_screen(struct fb_info *info)
{
	struct screen *sc;

	sc = xzalloc(sizeof(*sc));

	sc->s.x = 0;
	sc->s.y = 0;
	sc->s.width = info->xres;
	sc->s.height = info->yres;
	sc->fbsize = info->line_length * sc->s.height;
	sc->fb = info->screen_base;
	sc->info = info;

	return sc;
}

struct screen *fb_open(const char * fbdev)
{
	int fd, ret;
	struct fb_info *info;
	struct screen *sc;

	fd = open(fbdev, O_RDWR);
	if (fd < 0)
		return ERR_PTR(fd);

	info = xzalloc(sizeof(*info));

	ret = ioctl(fd, FBIOGET_SCREENINFO, &info);
	if (ret) {
		goto failed_screeninfo;
	}

	sc = fb_create_screen(info);
	if (IS_ERR(sc)) {
		ret = PTR_ERR(sc);
		goto failed_create;
	}

	sc->fd = fd;
	sc->info = info;

	return sc;

failed_create:
	free(sc);
failed_screeninfo:
	close(fd);

	return ERR_PTR(ret);
}

void fb_close(struct screen *sc)
{
	if (sc->fd > 0)
		close(sc->fd);

	free(sc);
}

void gu_screen_blit_area(struct screen *sc, int startx, int starty, int width,
		int height)
{
	struct fb_info *info = sc->info;
	int bpp = info->bits_per_pixel >> 3;

	if (info->screen_base_shadow) {
		int y;
		void *fb = info->screen_base + starty * sc->info->line_length + startx * bpp;
		void *fboff = info->screen_base_shadow + starty * sc->info->line_length + startx * bpp;

		for (y = starty; y < starty + height; y++) {
			memcpy(fb, fboff, width * bpp);
			fb += sc->info->line_length;
			fboff += sc->info->line_length;
		}
	}
}

void gu_screen_blit(struct screen *sc)
{
	struct fb_info *info = sc->info;

	if (info->screen_base_shadow)
		memcpy(info->screen_base, info->screen_base_shadow, sc->fbsize);
}
