#include <common.h>
#include <fb.h>
#include <graphic_utils.h>

static u32 get_pixel(struct fb_info *info, u32 color)
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

	px = (t >> (8 - info->transp.length)) << info->transp.offset |
		 (r >> (8 - info->red.length)) << info->red.offset |
		 (g >> (8 - info->green.length)) << info->green.offset |
		 (b >> (8 - info->blue.length)) << info->blue.offset;

	return px;
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

void memset_pixel(struct fb_info *info, void* buf, u32 color, size_t size)
{
	u32 px;
	u8 *screen = buf;

	px = get_pixel(info, color);

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

void set_pixel(struct fb_info *info, void *adr, u32 px)
{
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

void set_rgb_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b)
{
	u32 px;

	px = (r >> (8 - info->red.length)) << info->red.offset |
		(g >> (8 - info->green.length)) << info->green.offset |
		(b >> (8 - info->blue.length)) << info->blue.offset;

	set_pixel(info, adr, px);
}

static u8 alpha_mux(int s, int d, int a)
{
	return (d * a + s * (255 - a)) >> 8;
}

void set_rgba_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b, u8 a)
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

			set_rgb_pixel(info, adr, r, g, b);

			return;
		}
	}

	px |= (r >> (8 - info->red.length)) << info->red.offset |
		(g >> (8 - info->green.length)) << info->green.offset |
		(b >> (8 - info->blue.length)) << info->blue.offset;

	set_pixel(info, adr, px);
}

void rgba_blend(struct fb_info *info, void *image, void* buf, int height,
	int width, int startx, int starty, bool is_rgba)
{
	unsigned char *adr;
	int x, y;
	int xres;
	int img_byte_per_pixel = 3;

	if (is_rgba)
		img_byte_per_pixel++;

	xres = info->xres;

	for (y = 0; y < height; y++) {
		adr = buf + ((y + starty) * xres + startx) *
				(info->bits_per_pixel >> 3);

		for (x = 0; x < width; x++) {
			char *pixel;

			pixel = image;
			if (is_rgba)
				set_rgba_pixel(info, adr, pixel[0], pixel[1],
						pixel[2], pixel[3]);
			else
				set_rgb_pixel(info, adr, pixel[0], pixel[1],
						pixel[2]);
			adr += info->bits_per_pixel >> 3;
			image += img_byte_per_pixel;
		}
	}
}
