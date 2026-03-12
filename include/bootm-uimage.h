/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTM_UIMAGE_H
#define __BOOTM_UIMAGE_H

#include <linux/types.h>
#include <linux/err.h>

struct image_data;
struct fdt_header;
struct resource;

#ifdef CONFIG_BOOTM_UIMAGE

int bootm_open_uimage(struct image_data *data);

int bootm_collect_uimage_loadables(struct image_data *data);

#else

static inline int bootm_open_uimage(struct image_data *data)
{
	return -ENOSYS;
}

static inline int bootm_collect_uimage_loadables(struct image_data *data)
{
	return 0;
}

#endif

#endif
