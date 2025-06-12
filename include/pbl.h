/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#ifndef __PBL_H__
#define __PBL_H__

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <linux/compiler.h>

extern unsigned long free_mem_ptr;
extern unsigned long free_mem_end_ptr;

void pbl_barebox_uncompress(void *dest, void *compressed_start, unsigned int len);

void fdt_find_mem(const void *fdt, unsigned long *membase, unsigned long *memsize);
int fdt_fixup_mem(void *fdt, unsigned long membase[], unsigned long memsize[], size_t num);

struct fdt_device_id {
	const char *compatible;
	const void *data;
};

const void *
fdt_device_get_match_data(const void *fdt, const char *nodepath,
			  const struct fdt_device_id ids[]);

int pbl_barebox_verify(const void *compressed_start, unsigned int len,
		       const void *hash, unsigned int hash_len);
#endif

void __noreturn barebox_pbl_entry(ulong, ulong, void *);


#endif /* __PBL_H__ */
