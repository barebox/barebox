/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * Ext4 Extent data structures are taken from  original ext4 fs code
 * as found in the linux kernel.
 *
 * Copyright (c) 2003-2006, Cluster File Systems, Inc, info@clusterfs.com
 * Written by Alex Tomas <alex@clusterfs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __EXT4__
#define __EXT4__

#define EXT4_EXTENTS_FL		0x00080000 /* Inode uses extents */
#define EXT4_EXT_MAGIC			0xf30a
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM	0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS	0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT	0x0080
#define EXT4_INDIRECT_BLOCKS		12

#define EXT4_BG_INODE_UNINIT		0x0001
#define EXT4_BG_BLOCK_UNINIT		0x0002
#define EXT4_BG_INODE_ZEROED		0x0004

/*
 * ext4_inode has i_block array (60 bytes total).
 * The first 12 bytes store ext4_extent_header;
 * the remainder stores an array of ext4_extent.
 */

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
struct ext4_extent {
	__le32	ee_block;	/* first logical block extent covers */
	__le16	ee_len;		/* number of blocks covered by extent */
	__le16	ee_start_hi;	/* high 16 bits of physical block */
	__le32	ee_start_lo;	/* low 32 bits of physical block */
};

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
struct ext4_extent_idx {
	__le32	ei_block;	/* index covers logical blocks from 'block' */
	__le32	ei_leaf_lo;	/* pointer to the physical block of the next *
				 * level. leaf or next index could be there */
	__le16	ei_leaf_hi;	/* high 16 bits of physical block */
	__u16	ei_unused;
};

/* Each block (leaves and indexes), even inode-stored has header. */
struct ext4_extent_header {
	__le16	eh_magic;	/* probably will support different formats */
	__le16	eh_entries;	/* number of valid entries */
	__le16	eh_max;		/* capacity of store in entries */
	__le16	eh_depth;	/* has tree real underlying blocks? */
	__le32	eh_generation;	/* generation of the tree */
};

struct ext_filesystem {
	/* Inode size of partition */
	uint32_t inodesz;
	/* Group Descriptor size */
	uint16_t gdsize;

	/* Block Device Descriptor */
	struct cdev *cdev;

	struct ext2_data *data;

	struct device_d *dev;
};

struct ext2fs_node;

int ext4fs_open(struct ext2_data *data, const char *filename, struct ext2fs_node **inode);
int ext4fs_read(char *buf, unsigned len);
int ext4fs_mount(struct ext_filesystem *fs);
void ext4fs_umount(struct ext_filesystem *fs);
char *ext4fs_read_symlink(struct ext2fs_node *node);
void ext4fs_free_node(struct ext2fs_node *node, struct ext2fs_node *currroot);
ssize_t ext4fs_devread(struct ext_filesystem *fs, sector_t sector, int byte_offset, size_t byte_len, char *buf);
long int read_allocated_block(struct ext2fs_node *node, int fileblock);

#endif
