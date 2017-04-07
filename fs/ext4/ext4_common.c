/*
 * (C) Copyright 2011 - 2012 Samsung Electronics
 * EXT4 filesystem implementation in Uboot by
 * Uma Shankar <uma.shankar@samsung.com>
 * Manjunatha C Achar <a.manjunatha@samsung.com>
 *
 * ext4ls and ext4load : Based on ext2 ls load support in Uboot.
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

#include <common.h>
#include <malloc.h>
#include <linux/stat.h>
#include <linux/time.h>
#include <linux/magic.h>
#include <asm/byteorder.h>
#include <dma.h>

#include "ext4_common.h"

static struct ext4_extent_header *ext4fs_get_extent_block(struct ext2_data *data,
		char *buf, struct ext4_extent_header *ext_block,
		uint32_t fileblock, int log2_blksz)
{
	struct ext4_extent_idx *index;
	unsigned long long block;
	struct ext_filesystem *fs = data->fs;
	int blksz = EXT2_BLOCK_SIZE(data);
	int i, ret;

	while (1) {
		index = (struct ext4_extent_idx *)(ext_block + 1);

		if (le16_to_cpu(ext_block->eh_magic) != EXT4_EXT_MAGIC)
			return 0;

		if (ext_block->eh_depth == 0)
			return ext_block;
		i = -1;
		do {
			i++;
			if (i >= le16_to_cpu(ext_block->eh_entries))
				break;
		} while (fileblock >= le32_to_cpu(index[i].ei_block));

		if (--i < 0)
			return 0;

		block = le16_to_cpu(index[i].ei_leaf_hi);
		block = (block << 32) + le32_to_cpu(index[i].ei_leaf_lo);

		ret = ext4fs_devread(fs, block << log2_blksz, 0, blksz, buf);
		if (ret)
			return NULL;
		else
			ext_block = (struct ext4_extent_header *)buf;
	}
}

static int ext4fs_blockgroup(struct ext2_data *data, int group,
		struct ext2_block_group *blkgrp)
{
	long int blkno;
	unsigned int blkoff, desc_per_blk;
	struct ext_filesystem *fs = data->fs;
	int desc_size = fs->gdsize;

	desc_per_blk = EXT2_BLOCK_SIZE(data) / desc_size;

	blkno = le32_to_cpu(data->sblock.first_data_block) + 1 +
			group / desc_per_blk;
	blkoff = (group % desc_per_blk) * desc_size;

	dev_dbg(fs->dev, "read %d group descriptor (blkno %ld blkoff %u)\n",
	      group, blkno, blkoff);

	return ext4fs_devread(fs, blkno << LOG2_EXT2_BLOCK_SIZE(data),
				blkoff, desc_size, (char *)blkgrp);
}

int ext4fs_read_inode(struct ext2_data *data, int ino, struct ext2_inode *inode)
{
	struct ext2_block_group blkgrp;
	struct ext2_sblock *sblock = &data->sblock;
	struct ext_filesystem *fs = data->fs;
	int inodes_per_block, ret;
	long int blkno;
	unsigned int blkoff;

	/* It is easier to calculate if the first inode is 0. */
	ino--;
	ret = ext4fs_blockgroup(data, ino / le32_to_cpu
				   (sblock->inodes_per_group), &blkgrp);
	if (ret)
		return ret;

	inodes_per_block = EXT2_BLOCK_SIZE(data) / fs->inodesz;
	blkno = le32_to_cpu(blkgrp.inode_table_id) +
	    (ino % le32_to_cpu(sblock->inodes_per_group)) / inodes_per_block;
	blkoff = (ino % inodes_per_block) * fs->inodesz;
	/* Read the inode. */
	ret = ext4fs_devread(fs, blkno << LOG2_EXT2_BLOCK_SIZE(data), blkoff,
				sizeof(struct ext2_inode), (char *)inode);
	if (ret)
		return ret;

	return 0;
}

static int ext4fs_get_indir_block(struct ext2fs_node *node,
				struct ext4fs_indir_block *indir, int blkno)
{
	struct ext_filesystem *fs = node->data->fs;
	int blksz;
	int ret;

	blksz = EXT2_BLOCK_SIZE(node->data);

	if (indir->blkno == blkno)
		return 0;

	ret = ext4fs_devread(fs, blkno, 0, blksz, (void *)indir->data);
	if (ret) {
		dev_err(fs->dev, "** SI ext2fs read block (indir 1)"
			"failed. **\n");
		return ret;
	}

	return 0;
}

long int read_allocated_block(struct ext2fs_node *node, int fileblock)
{
	long int blknr;
	int blksz;
	int log2_blksz;
	long int rblock;
	long int perblock_parent;
	long int perblock_child;
	unsigned long long start;
	struct ext2_inode *inode = &node->inode;
	struct ext2_data *data = node->data;
	int ret;

	/* get the blocksize of the filesystem */
	blksz = EXT2_BLOCK_SIZE(node->data);
	log2_blksz = LOG2_EXT2_BLOCK_SIZE(node->data);

	if (le32_to_cpu(inode->flags) & EXT4_EXTENTS_FL) {
		char *buf = zalloc(blksz);
		struct ext4_extent_header *ext_block;
		struct ext4_extent *extent;
		int i = -1;

		if (!buf)
			return -ENOMEM;

		ext_block = ext4fs_get_extent_block(node->data, buf,
				(struct ext4_extent_header *)inode->b.blocks.dir_blocks,
				fileblock, log2_blksz);
		if (!ext_block) {
			pr_err("invalid extent block\n");
			free(buf);
			return -EINVAL;
		}

		extent = (struct ext4_extent *)(ext_block + 1);

		do {
			i++;
			if (i >= le16_to_cpu(ext_block->eh_entries))
				break;
		} while (fileblock >= le32_to_cpu(extent[i].ee_block));

		if (--i >= 0) {
			fileblock -= le32_to_cpu(extent[i].ee_block);
			if (fileblock >= le16_to_cpu(extent[i].ee_len)) {
				free(buf);
				return 0;
			}

			start = le16_to_cpu(extent[i].ee_start_hi);
			start = (start << 32) +
					le32_to_cpu(extent[i].ee_start_lo);
			free(buf);
			return fileblock + start;
		}

		free(buf);
		return -EIO;
	}

	if (fileblock < INDIRECT_BLOCKS) {
		/* Direct blocks. */
		blknr = le32_to_cpu(inode->b.blocks.dir_blocks[fileblock]);
	} else if (fileblock < (INDIRECT_BLOCKS + (blksz / 4))) {
		/* Indirect. */
		ret = ext4fs_get_indir_block(node, &data->indir1,
				le32_to_cpu(inode->b.blocks.indir_block) << log2_blksz);
		if (ret)
			return ret;
		blknr = le32_to_cpu(data->indir1.data[fileblock - INDIRECT_BLOCKS]);
	} else if (fileblock < (INDIRECT_BLOCKS + (blksz / 4 *
					(blksz / 4 + 1)))) {
		/* Double indirect. */
		long int perblock = blksz / 4;
		long int rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4);

		ret = ext4fs_get_indir_block(node, &data->indir1,
				le32_to_cpu(inode->b.blocks.double_indir_block) << log2_blksz);
		if (ret)
			return ret;

		ret = ext4fs_get_indir_block(node, &data->indir2,
				le32_to_cpu(data->indir1.data[rblock / perblock]) << log2_blksz);
		if (ret)
			return ret;

		blknr = le32_to_cpu(data->indir2.data[rblock % perblock]);
	} else {
		/* Triple indirect. */
		rblock = fileblock - (INDIRECT_BLOCKS + blksz / 4 +
				      (blksz / 4 * blksz / 4));
		perblock_child = blksz / 4;
		perblock_parent = ((blksz / 4) * (blksz / 4));

		ret = ext4fs_get_indir_block(node, &data->indir1,
				le32_to_cpu(inode->b.blocks.triple_indir_block) << log2_blksz);
		if (ret)
			return ret;

		ret = ext4fs_get_indir_block(node, &data->indir2,
				le32_to_cpu(data->indir1.data[rblock / perblock_parent]) << log2_blksz);
		if (ret)
			return ret;

		ret = ext4fs_get_indir_block(node, &data->indir3,
				le32_to_cpu(data->indir2.data[rblock / perblock_child]) << log2_blksz);
		if (ret)
			return ret;

		blknr = le32_to_cpu(data->indir3.data[rblock % perblock_child]);
	}

	return blknr;
}

int ext4fs_iterate_dir(struct ext2fs_node *dir, char *name,
				struct ext2fs_node **fnode, int *ftype)
{
	unsigned int fpos = 0;
	int status, ret;
	struct ext2fs_node *diro = (struct ext2fs_node *) dir;
	struct ext_filesystem *fs = dir->data->fs;

	if (name != NULL)
		dev_dbg(fs->dev, "Iterate dir %s\n", name);

	if (!diro->inode_read) {
		ret = ext4fs_read_inode(diro->data, diro->ino, &diro->inode);
		if (ret)
			return ret;
	}
	/* Search the file.  */
	while (fpos < le32_to_cpu(diro->inode.size)) {
		struct ext2_dirent dirent;

		status = ext4fs_read_file(diro, fpos,
					   sizeof(struct ext2_dirent),
					   (char *) &dirent);
		if (status < 1)
			return -EINVAL;

		if (dirent.namelen != 0) {
			char filename[dirent.namelen + 1];
			struct ext2fs_node *fdiro;
			int type = FILETYPE_UNKNOWN;

			status = ext4fs_read_file(diro,
						  fpos +
						  sizeof(struct ext2_dirent),
						  dirent.namelen, filename);
			if (status < 1)
				return -EINVAL;

			fdiro = zalloc(sizeof(struct ext2fs_node));
			if (!fdiro)
				return -ENOMEM;

			fdiro->data = diro->data;
			fdiro->ino = le32_to_cpu(dirent.inode);

			filename[dirent.namelen] = '\0';

			if (dirent.filetype != FILETYPE_UNKNOWN) {
				fdiro->inode_read = 0;

				if (dirent.filetype == FILETYPE_DIRECTORY)
					type = FILETYPE_DIRECTORY;
				else if (dirent.filetype == FILETYPE_SYMLINK)
					type = FILETYPE_SYMLINK;
				else if (dirent.filetype == FILETYPE_REG)
					type = FILETYPE_REG;
			} else {
				ret = ext4fs_read_inode(diro->data,
							   le32_to_cpu
							   (dirent.inode),
							   &fdiro->inode);
				if (ret) {
					free(fdiro);
					return ret;
				}
				fdiro->inode_read = 1;

				if ((le16_to_cpu(fdiro->inode.mode) &
				     FILETYPE_INO_MASK) ==
				    FILETYPE_INO_DIRECTORY) {
					type = FILETYPE_DIRECTORY;
				} else if ((le16_to_cpu(fdiro->inode.mode)
					    & FILETYPE_INO_MASK) ==
					   FILETYPE_INO_SYMLINK) {
					type = FILETYPE_SYMLINK;
				} else if ((le16_to_cpu(fdiro->inode.mode)
					    & FILETYPE_INO_MASK) ==
					   FILETYPE_INO_REG) {
					type = FILETYPE_REG;
				}
			}

			dev_dbg(fs->dev, "iterate >%s<\n", filename);

			if (strcmp(filename, name) == 0) {
				*ftype = type;
				*fnode = fdiro;
				return 0;
			}

			free(fdiro);
		}
		fpos += le16_to_cpu(dirent.direntlen);
	}
	return -ENOENT;
}

char *ext4fs_read_symlink(struct ext2fs_node *node)
{
	char *symlink;
	struct ext2fs_node *diro = node;
	int status, ret;

	if (!diro->inode_read) {
		ret = ext4fs_read_inode(diro->data, diro->ino, &diro->inode);
		if (ret)
			return NULL;
	}
	symlink = zalloc(le32_to_cpu(diro->inode.size) + 1);
	if (!symlink)
		return 0;

	if (le32_to_cpu(diro->inode.size) < sizeof(diro->inode.b.symlink)) {
		strncpy(symlink, diro->inode.b.symlink,
			 le32_to_cpu(diro->inode.size));
	} else {
		status = ext4fs_read_file(diro, 0,
					   le32_to_cpu(diro->inode.size),
					   symlink);
		if (status == 0) {
			free(symlink);
			return NULL;
		}
	}

	symlink[le32_to_cpu(diro->inode.size)] = '\0';

	return symlink;
}

int ext4fs_find_file(const char *currpath,
			     struct ext2fs_node *currroot,
			     struct ext2fs_node **currfound, int *foundtype)
{
	char fpath[strlen(currpath) + 1];
	char *name = fpath;
	char *next;
	int type = FILETYPE_DIRECTORY;
	struct ext2fs_node *currnode = currroot;
	struct ext2fs_node *oldnode = currroot;
	int ret = 0;

	strncpy(fpath, currpath, strlen(currpath) + 1);

	/* Remove all leading slashes. */
	while (*name == '/')
		name++;

	if (!*name) {
		*currfound = currnode;
		goto out;
	}

	for (;;) {
		/* Extract the actual part from the pathname. */
		next = strchr(name, '/');
		if (next) {
			/* Remove all leading slashes. */
			while (*next == '/')
				*(next++) = '\0';
		}

		if (type != FILETYPE_DIRECTORY) {
			ext4fs_free_node(currnode, currroot);
			return -ENOENT;
		}

		oldnode = currnode;

		/* Iterate over the directory. */
		ret = ext4fs_iterate_dir(currnode, name, &currnode, &type);
		if (ret)
			return ret;

		ext4fs_free_node(oldnode, currroot);

		/* Found the node! */
		if (!next || *next == '\0') {
			*currfound = currnode;
			goto out;
		}
		name = next;
	}

out:
	if (foundtype)
		*foundtype = type;

	return ret;
}

int ext4fs_open(struct ext2_data *data, const char *filename, struct ext2fs_node **inode)
{
	struct ext2fs_node *fdiro = NULL;
	int status, ret;
	int type;

	status = ext4fs_find_file(filename, &data->diropen, &fdiro, &type);
	if (status)
		goto fail;

	if (type != FILETYPE_REG)
		return -EINVAL;

	if (!fdiro->inode_read) {
		ret = ext4fs_read_inode(fdiro->data, fdiro->ino,
				&fdiro->inode);
		if (ret)
			goto fail;
	}

	*inode = fdiro;

	return 0;
fail:
	ext4fs_free_node(fdiro, &data->diropen);

	return -ENOENT;
}

int ext4fs_mount(struct ext_filesystem *fs)
{
	struct ext2_data *data;
	int ret, blksz;

	data = zalloc(sizeof(struct ext2_data));
	if (!data)
		return -ENOMEM;

	/* Read the superblock. */
	ret = ext4fs_devread(fs, 1 * 2, 0, sizeof(struct ext2_sblock),
				(char *)&data->sblock);
	if (ret)
		goto fail;

	/* Make sure this is an ext2 filesystem. */
	if (le16_to_cpu(data->sblock.magic) != EXT2_SUPER_MAGIC) {
		ret = -EINVAL;
		goto fail;
	}

	if (le32_to_cpu(data->sblock.revision_level) == 0) {
		fs->inodesz = 128;
		fs->gdsize = 32;
	} else {
		debug("EXT4 features COMPAT: %08x INCOMPAT: %08x RO_COMPAT: %08x\n",
		      le32_to_cpu(data->sblock.feature_compatibility),
		      le32_to_cpu(data->sblock.feature_incompat),
		      le32_to_cpu(data->sblock.feature_ro_compat));

		fs->inodesz = le16_to_cpu(data->sblock.inode_size);
		fs->gdsize = le32_to_cpu(data->sblock.feature_incompat) &
			EXT4_FEATURE_INCOMPAT_64BIT ?
			le16_to_cpu(data->sblock.descriptor_size) : 32;
	}

	dev_info(fs->dev, "EXT2 rev %d, inode_size %d, descriptor size %d\n",
	      le32_to_cpu(data->sblock.revision_level),
	      fs->inodesz, fs->gdsize);

	data->diropen.data = data;
	data->diropen.ino = 2;
	data->diropen.inode_read = 1;
	data->inode = &data->diropen.inode;
	data->fs = fs;
	fs->data = data;

	blksz = EXT2_BLOCK_SIZE(data);

	fs->data->indir1.data = malloc(blksz);
	fs->data->indir2.data = malloc(blksz);
	fs->data->indir3.data = malloc(blksz);

	if (!fs->data->indir1.data || !fs->data->indir2.data ||
			!fs->data->indir3.data) {
		ret = -ENOMEM;
		goto fail;
	}

	ret = ext4fs_read_inode(data, 2, data->inode);
	if (ret)
		goto fail;

	return 0;
fail:
	free(data);

	return ret;
}

void ext4fs_umount(struct ext_filesystem *fs)
{
	free(fs->data->indir1.data);
	free(fs->data->indir2.data);
	free(fs->data->indir3.data);
	free(fs->data);
}
