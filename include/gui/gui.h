/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __GUI_H__
#define __GUI_H__

#include <fb.h>

struct surface {
	int x;
	int y;
	int width;
	int height;
};

struct screen {
	int fd;
	struct fb_info *info;

	struct surface s;

	void *fb;
	int fbsize;
};

static inline void *gui_screen_render_buffer(struct screen *sc)
{
	return fb_get_screen_base(sc->info);
}

#endif /* __GUI_H__ */
