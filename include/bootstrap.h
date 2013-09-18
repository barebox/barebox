/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#ifndef __BOOSTRAP_H__
#define __BOOSTRAP_H__

#include <common.h>

#define bootstrap_err(fmt, arg...) printf(fmt, ##arg)

void bootstrap_boot(int (*func)(void), bool barebox);

#ifdef CONFIG_BOOTSTRAP_DEVFS
void* bootstrap_read_devfs(char *devname, bool use_bb, int offset,
			   int default_size, int max_size);
#else
static inline void* bootstrap_read_devfs(char *devname, bool use_bb, int offset,
			   int default_size, int max_size)
{
	return NULL;
}
#endif

#ifdef CONFIG_BOOTSTRAP_DISK
void* bootstrap_read_disk(char *devname, char *fstype);
#else
static inline void* bootstrap_read_disk(char *devname, char *fstype)
{
	return NULL;
}
#endif

#endif /* __BOOSTRAP_H__ */
