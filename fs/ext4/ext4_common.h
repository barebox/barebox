/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * ext4ls and ext4load :  based on ext2 ls load support in Uboot.
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * based on code from grub2 fs/ext2.c and fs/fshelp.c by
 * GRUB  --  GRand Unified Bootloader
 * Copyright (C) 2003, 2004  Free Software Foundation, Inc.
 *
 * ext4write : Based on generic ext4 protocol.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __EXT4_COMMON__
#define __EXT4_COMMON__
#include <malloc.h>
#include <errno.h>
#include <dma.h>
#include "ext4fs.h"
#include "ext_common.h"

static inline void *zalloc(size_t size)
{
	void *p = dma_alloc(size);

	if (p)
		memset(p, 0, size);

	return p;
}

int ext4fs_read_inode(struct ext2_data *data, int ino,
		      struct ext2_inode *inode);
int ext4fs_read_file(struct ext2fs_node *node, int pos,
		unsigned int len, char *buf);
int ext4fs_find_file(const char *path, struct ext2fs_node *rootnode,
			struct ext2fs_node **foundnode, int *foundtype);
int ext4fs_iterate_dir(struct ext2fs_node *dir, char *name,
			struct ext2fs_node **fnode, int *ftype);

#endif
