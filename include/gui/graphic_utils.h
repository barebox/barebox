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

void rgba_blend(struct fb_info *info, struct image *img, void* dest, int height,
	int width, int startx, int starty, bool is_rgba);
void set_pixel(struct fb_info *info, void *adr, u32 px);
void set_rgb_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b);
void set_rgba_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b, u8 a);
void memset_pixel(struct fb_info *info, void* buf, u32 color, size_t size);
int fb_open(const char * fbdev, struct screen *sc, bool offscreen);
void fb_close(struct screen *sc);
void screen_blit(struct screen *sc);

#endif /* __GRAPHIC_UTILS_H__ */
