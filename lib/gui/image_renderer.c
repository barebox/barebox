/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#include <common.h>
#include <fb.h>
#include <gui/image_renderer.h>
#include <errno.h>
#include <fs.h>
#include <malloc.h>

static LIST_HEAD(image_renderers);

static struct image_renderer *get_renderer(void* buf, size_t bufsize)
{
	struct image_renderer *ir;
	enum filetype type = file_detect_type(buf, bufsize);

	list_for_each_entry(ir, &image_renderers, list) {
		if (ir->type == type)
			return ir;
	}

	return NULL;
}

struct image *image_renderer_open(const char* file)
{
	void *data;
	size_t size;
	struct image_renderer *ir;
	struct image *img;
	int ret;

	data = read_file(file, &size);
	if (!data) {
		printf("unable to read %s\n", file);
		return ERR_PTR(-ENOMEM);
	}

	ir = get_renderer(data, size);
	if (!ir) {
		ret = -ENOENT;
		goto out;
	}

	img = ir->open(data, size);
	if (IS_ERR(img)) {
		ret = PTR_ERR(img);
		goto out;
	}
	img->ir = ir;
	if (!ir->keep_file_data)
		free(data);

	return img;
out:
	free(data);
	return ERR_PTR(ret);
}

void image_renderer_close(struct image *img)
{
	if (!img)
		return;

	img->ir->close(img);

	free(img);
}

int image_renderer_image(struct screen *sc, struct surface *s, struct image *img)
{
	return img->ir->renderer(sc, s, img);
}

int image_renderer_register(struct image_renderer *ir)
{
	if (!ir || !ir->type || !ir->renderer || !ir->open || !ir->close)
		return -EIO;

	list_add_tail(&ir->list, &image_renderers);

	return 0;
}

void image_renderer_unregister(struct image_renderer *ir)
{
	if (!ir)
		return;

	list_del(&ir->list);
}
