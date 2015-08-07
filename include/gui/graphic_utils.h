/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __GRAPHIC_UTILS_H__
#define __GRAPHIC_UTILS_H__

#include <fb.h>
#include <gui/image.h>
#include <gui/gui.h>

u32 gu_hex_to_pixel(struct fb_info *info, u32 color);
u32 gu_rgb_to_pixel(struct fb_info *info, u8 r, u8 g, u8 b, u8 t);
void gu_rgba_blend(struct fb_info *info, struct image *img, void* dest, int height,
	int width, int startx, int starty, bool is_rgba);
void gu_set_pixel(struct fb_info *info, void *adr, u32 px);
void gu_set_rgb_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b);
void gu_set_rgba_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b, u8 a);
void gu_memset_pixel(struct fb_info *info, void* buf, u32 color, size_t size);
struct screen *fb_create_screen(struct fb_info *info);
struct screen *fb_open(const char *fbdev);
void fb_close(struct screen *sc);
void gu_screen_blit(struct screen *sc);
void gu_invert_area(struct fb_info *info, void *buf, int startx, int starty, int width,
		int height);
void gu_screen_blit_area(struct screen *sc, int startx, int starty, int width,
		int height);

#endif /* __GRAPHIC_UTILS_H__ */
