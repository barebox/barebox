/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 */

#ifndef __BOOSTRAP_H__
#define __BOOSTRAP_H__

#include <common.h>

#define bootstrap_err(fmt, arg...) printf(fmt, ##arg)

typedef void (*kernel_entry_func)(int zero, unsigned arch, void *params);
void bootstrap_boot(kernel_entry_func func, bool barebox);

#ifdef CONFIG_BOOTSTRAP_DEVFS
void* bootstrap_read_devfs(char *devname, bool use_bb, int offset,
			   int default_size, int max_size, size_t *bufsize);
#else
static inline void* bootstrap_read_devfs(char *devname, bool use_bb, int offset,
			   int default_size, int max_size, size_t *bufsize)
{
	return NULL;
}
#endif

#ifdef CONFIG_BOOTSTRAP_DISK
void* bootstrap_read_disk(const char *devname, char *fstype, size_t *bufsize);
#else
static inline void* bootstrap_read_disk(const char *devname, char *fstype, size_t *bufsize)
{
	return NULL;
}
#endif

#endif /* __BOOSTRAP_H__ */
