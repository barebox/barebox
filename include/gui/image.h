/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __GUI_IMAGE_H__
#define __GUI_IMAGE_H__

struct image_renderer;

struct image {
	void *data;
	struct image_renderer *ir;
	int height;
	int width;
	int bits_per_pixel;
};

#endif /* __IMAGE_RENDERER_H__ */
