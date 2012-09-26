/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __PNG_H__
#define __PNG_H__

#include <filetype.h>
#include <linux/list.h>
#include <errno.h>
#include <linux/err.h>

int png_uncompress_init(void);
void png_uncompress_exit(void);
void png_close(struct image *img);
struct image *png_open(char *inbuf, int insize);
extern z_stream png_stream;

#endif /* __PNG_H__ */
