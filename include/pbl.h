/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#ifndef __PBL_H__
#define __PBL_H__

#include <linux/types.h>

extern unsigned long free_mem_ptr;
extern unsigned long free_mem_end_ptr;

void pbl_barebox_uncompress(void *dest, void *compressed_start, unsigned int len);

#ifdef __PBL__
#define IN_PBL	1

struct pbl_bio {
	void *priv;
	int (*read)(struct pbl_bio *bio, off_t block_off, void *buf, unsigned nblocks);
};

static inline int pbl_bio_read(struct pbl_bio *bio, off_t block_off,
			       void *buf, unsigned nblocks)
{
	return bio->read(bio, block_off, buf, nblocks);
}

ssize_t pbl_fat_load(struct pbl_bio *, const char *filename, void *dest, size_t len);
#else
#define IN_PBL	0
#endif

void fdt_find_mem(const void *fdt, unsigned long *membase, unsigned long *memsize);

struct fdt_device_id {
	const char *compatible;
	const void *data;
};

const void *
fdt_device_get_match_data(const void *fdt, const char *nodepath,
			  const struct fdt_device_id ids[]);

#endif /* __PBL_H__ */
