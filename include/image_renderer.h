/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __IMAGE_RENDER_H__
#define __IMAGE_RENDER_H__

#include <filetype.h>
#include <linux/list.h>
#include <errno.h>
#include <linux/err.h>
#include <fb.h>

struct image {
	void *data;
	struct image_renderer *ir;
	int height;
	int width;
	int bits_per_pixel;
};

struct image_renderer {
	enum filetype type;
	struct image *(*open)(char *data, int size);
	void (*close)(struct image *img);
	int (*renderer)(struct fb_info *info, struct image *img, void* fb,
		int startx, int starty, void* offscreenbuf);

	/*
	 * do not free the data read from the file
	 * needed by bmp support
	 */
	int keep_file_data;

	struct list_head list;
};

#ifdef CONFIG_IMAGE_RENDERER
int image_renderer_register(struct image_renderer *ir);
void image_render_unregister(struct image_renderer *ir);

int image_renderer_image(struct fb_info *info, struct image *img, void* fb,
	    int startx, int starty, void* offscreenbuf);

struct image *image_renderer_open(const char* file);
void image_renderer_close(struct image *img);

#else
static inline int image_renderer_register(struct image_renderer *ir)
{
	return -EINVAL;
}
static inline void image_renderer_unregister(struct image_renderer *ir) {}

static inline struct image *image_renderer_open(const char* file)
{
	return ERR_PTR(-EINVAL);
}

static inline void image_renderer_close(struct image *img) {}

int image_renderer_image(struct fb_info *info, struct image *img, void* fb,
	    int startx, int starty, void* offscreenbuf);
#endif

static inline int image_renderer_file(struct fb_info *info, const char* file, void* fb,
		    int startx, int starty, void* offscreenbuf)
{
	struct image* img = image_renderer_open(file);
	int ret;

	if (IS_ERR(img))
		return PTR_ERR(img);

	ret = image_renderer_image(info, img, fb, startx, starty,
				offscreenbuf);

	image_renderer_close(img);

	return ret; 
}

#endif /* __IMAGE_RENDERER_H__ */
