/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __GRAPHIC_UTILS_H__
#define __GRAPHIC_UTILS_H__

void set_pixel(struct fb_info *info, void *adr, u32 px);
void set_rgb_pixel(struct fb_info *info, void *adr, u8 r, u8 g, u8 b);
void memset_pixel(struct fb_info *info, void* buf, u32 color, size_t size);

#endif /* __GRAPHIC_UTILS_H__ */
