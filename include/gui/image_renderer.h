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
#include <gui/image.h>
#include <gui/gui.h>

struct image_renderer {
	enum filetype type;
	struct image *(*open)(char *data, int size);
	void (*close)(struct image *img);
	int (*renderer)(struct screen *sc, struct surface *s, struct image *img);

	/*
	 * do not free the data read from the file
	 * needed by bmp support
	 */
	int keep_file_data;

	struct list_head list;
};

#ifdef CONFIG_IMAGE_RENDERER
int image_renderer_register(struct image_renderer *ir);
void image_renderer_unregister(struct image_renderer *ir);

int image_renderer_image(struct screen *sc, struct surface *s, struct image *img);

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

int image_renderer_image(struct surface *s, struct image *img);
#endif

static inline int image_renderer_file(struct screen *sc, struct surface *s, const char* file)
{
	struct image* img = image_renderer_open(file);
	int ret;

	if (IS_ERR(img))
		return PTR_ERR(img);

	ret = image_renderer_image(sc, s, img);

	image_renderer_close(img);

	return ret;
}

#endif /* __IMAGE_RENDERER_H__ */
