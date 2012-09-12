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
