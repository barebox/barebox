/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTM_UIMAGE_H
#define __BOOTM_UIMAGE_H

#include <linux/types.h>
#include <linux/err.h>

struct image_data;
struct fdt_header;
struct resource;

#ifdef CONFIG_BOOTM_UIMAGE

int bootm_load_uimage_os(struct image_data *data, unsigned long load_address);

struct resource *bootm_load_uimage_initrd(struct image_data *data,
					  unsigned long load_address);

int bootm_open_oftree_uimage(struct image_data *data, size_t *size,
			     struct fdt_header **fdt);
int bootm_open_uimage(struct image_data *data);

void bootm_close_uimage(struct image_data *data);

#else

static inline int bootm_load_uimage_os(struct image_data *data,
				       unsigned long load_address)
{
	return -ENOSYS;
}

static inline struct resource *
bootm_load_uimage_initrd(struct image_data *data, unsigned long load_address)
{
	return ERR_PTR(-ENOSYS);
}

static inline int bootm_open_oftree_uimage(struct image_data *data,
					   size_t *size,
					   struct fdt_header **fdt)
{
	return -ENOSYS;
}

static inline int bootm_open_uimage(struct image_data *data)
{
	return -ENOSYS;
}

static inline void bootm_close_uimage(struct image_data *data)
{
}

#endif

#endif
