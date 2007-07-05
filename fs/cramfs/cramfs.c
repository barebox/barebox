/*
 * cramfs.c
 *
 * Copyright (C) 1999 Linus Torvalds
 *
 * Copyright (C) 2000-2002 Transmeta Corporation
 *
 * Copyright (C) 2003 Kai-Uwe Bloem,
 * Auerswald GmbH & Co KG, <linux-development@auerswald.de>
 * - adapted from the www.tuxbox.org u-boot tree, added "ls" command
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * Compressed ROM filesystem for Linux.
 *
 * TODO:
 * add support for resolving symbolic links
 */

/*
 * These are the VFS interfaces to the compressed ROM filesystem.
 * The actual compression is based on zlib, see the other files.
 */

#include <common.h>
#include <malloc.h>
#include <driver.h>
#include <init.h>
#include <errno.h>
#include <fs.h>
#include <xfuncs.h>

#include <asm/byteorder.h>
#include <linux/stat.h>
#include <jffs2/jffs2.h>
#include <jffs2/load_kernel.h>
#include <cramfs/cramfs_fs.h>

/* These two macros may change in future, to provide better st_ino
   semantics. */
#define CRAMINO(x)	(CRAMFS_GET_OFFSET(x) ? CRAMFS_GET_OFFSET(x)<<2 : 1)
#define OFFSET(x)	((x)->i_ino)

struct cramfs_priv {
	struct cramfs_super super;
	int curr_block;
	struct cramfs_inode *inode;
	char buf[4096];
	int curr_block_len;
};

static int cramfs_read_super (struct device_d *dev, struct cramfs_priv *priv)
{
	unsigned long root_offset;
	struct cramfs_super *super = &priv->super;

	if (dev_read(dev, super, sizeof (struct cramfs_super), 0, 0) < sizeof (struct cramfs_super)) {
		printf("read superblock failed\n");
		return -EINVAL;
	}

	/* Do sanity checks on the superblock */
	if (super->magic != CRAMFS_32 (CRAMFS_MAGIC)) {
		/* check at 512 byte offset */
		if (dev_read(dev, super, sizeof (struct cramfs_super), 512, 0) < sizeof (struct cramfs_super)) {
			printf("read superblock failed\n");
			return -EINVAL;
		}
		if (super->magic != CRAMFS_32 (CRAMFS_MAGIC)) {
			printf ("cramfs: wrong magic\n");
			return -1;
		}
	}

	/* flags is reused several times, so swab it once */
	super->flags = CRAMFS_32 (super->flags);
	super->size = CRAMFS_32 (super->size);

	/* get feature flags first */
	if (super->flags & ~CRAMFS_SUPPORTED_FLAGS) {
		printf ("cramfs: unsupported filesystem features\n");
		return -1;
	}

	/* Check that the root inode is in a sane state */
	if (!S_ISDIR (CRAMFS_16 (super->root.mode))) {
		printf ("cramfs: root is not a directory\n");
		return -1;
	}
	root_offset = CRAMFS_GET_OFFSET (&(super->root)) << 2;
	if (root_offset == 0) {
		printf ("cramfs: empty filesystem");
	} else if (!(super->flags & CRAMFS_FLAG_SHIFTED_ROOT_OFFSET) &&
		   ((root_offset != sizeof (struct cramfs_super)) &&
		    (root_offset != 512 + sizeof (struct cramfs_super)))) {
		printf ("cramfs: bad root offset %lu\n", root_offset);
		return -1;
	}

	return 0;
}

static struct cramfs_inode *cramfs_get_inode(struct device_d *dev, unsigned long offset)
{
	struct cramfs_inode *inode = xmalloc(sizeof(struct cramfs_inode));

	if (dev_read(dev, inode, sizeof(struct cramfs_inode), offset, 0) < 0) {
		free(inode);
		return NULL;
	}

	return inode;
}

static struct cramfs_inode *cramfs_resolve (struct device_d *dev, unsigned long offset,
				     unsigned long size, int raw,
				     char *filename)
{
	unsigned long inodeoffset = 0, nextoffset;
	struct cramfs_inode *inode = NULL, *ret;
	char *name = xmalloc(256);

	while (inodeoffset < size) {
		int namelen;
		inode = cramfs_get_inode(dev, offset + inodeoffset);

		/*
		 * Namelengths on disk are shifted by two
		 * and the name padded out to 4-byte boundaries
		 * with zeroes.
		 */
		namelen = CRAMFS_GET_NAMELEN (inode) << 2;
		dev_read(dev, name, namelen, offset + inodeoffset + sizeof (struct cramfs_inode), 0);

		nextoffset =
			inodeoffset + sizeof (struct cramfs_inode) + namelen;

		if (!strncmp (filename, name, namelen)) {
			char *p = strtok (NULL, "/");

			if (raw && (p == NULL || *p == '\0'))
				goto out1;

			if (S_ISDIR (CRAMFS_16 (inode->mode))) {
				ret = cramfs_resolve (dev,
					CRAMFS_GET_OFFSET(inode) << 2,
					CRAMFS_24 (inode->size),
					raw, p);
				goto out;
			} else if (S_ISREG (CRAMFS_16 (inode->mode))) {
				goto out1;
			} else {
				printf ("%*.*s: unsupported file type (%x)\n",
					namelen, namelen, name,
					CRAMFS_16 (inode->mode));
				ret = NULL;
				goto out;
			}
		}

		free(inode);
		inodeoffset = nextoffset;
	}

	free(name);
	return NULL;

out1:
	ret = cramfs_get_inode(dev, offset + inodeoffset);
out:
	free(inode);
	free(name);
	return ret;
}

static int cramfs_fill_dirent (struct device_d *dev, unsigned long offset, struct dirent *d)
{
	struct cramfs_inode *inode = cramfs_get_inode(dev, offset);
	int namelen;

	if (!inode)
		return -EINVAL;

	memset(d->d_name, 0, 256);

	/*
	 * Namelengths on disk are shifted by two
	 * and the name padded out to 4-byte boundaries
	 * with zeroes.
	 */

	namelen = CRAMFS_GET_NAMELEN (inode) << 2;
	dev_read(dev, d->d_name, namelen, offset + sizeof(struct cramfs_inode), 0);
	free(inode);
	return namelen;
}

struct cramfs_dir {
	unsigned long offset, size;
	unsigned long inodeoffset;
	DIR dir;
};

DIR* cramfs_opendir(struct device_d *_dev, const char *filename)
{
	char *f;
	struct cramfs_priv *priv = _dev->priv;
	struct fs_device_d *fsdev = _dev->type_data;
	struct device_d *dev = fsdev->parent;

	struct cramfs_dir *dir = xzalloc(sizeof(struct cramfs_dir));
	dir->dir.priv = dir;

	if (strlen (filename) == 0 || !strcmp (filename, "/")) {
		/* Root directory. Use root inode in super block */
		dir->offset = CRAMFS_GET_OFFSET (&(priv->super.root)) << 2;
		dir->size = CRAMFS_24 (priv->super.root.size);
	} else {
		struct cramfs_inode *inode;

		f = strdup(filename);
		/* Resolve the path */
		inode = cramfs_resolve (dev,
					 CRAMFS_GET_OFFSET (&(priv->super.root)) <<
					 2, CRAMFS_24 (priv->super.root.size), 1,
					 strtok (f, "/"));
		free(f);
		if (!inode)
			goto err_free;

		/* Resolving was successful. Examine the inode */
		if (!S_ISDIR (CRAMFS_16 (inode->mode))) {
			/* It's not a directory */
			free(inode);
			goto err_free;
		}

		dir->offset = CRAMFS_GET_OFFSET (inode) << 2;
		dir->size = CRAMFS_24 (inode->size);
		free(inode);
	}

	return &dir->dir;

err_free:
	free(dir);
	return NULL;
}

static struct dirent* cramfs_readdir(struct device_d *_dev, DIR *_dir)
{
	struct fs_device_d *fsdev = _dev->type_data;
	struct device_d *dev = fsdev->parent;
	struct cramfs_dir *dir = _dir->priv;
	unsigned long nextoffset;

	/* List the given directory */
	if (dir->inodeoffset < dir->size) {
		nextoffset = cramfs_fill_dirent (dev, dir->offset + dir->inodeoffset, &_dir->d);

		dir->inodeoffset += sizeof (struct cramfs_inode) + nextoffset;
		return &_dir->d;
	}
	return NULL;
}

static int cramfs_closedir(struct device_d *dev, DIR *_dir)
{
	struct cramfs_dir *dir = _dir->priv;
	free(dir);
	return 0;
}

static int cramfs_open(struct device_d *_dev, FILE *file, const char *filename)
{
	struct cramfs_priv *priv = _dev->priv;
	struct cramfs_inode *inode;
	struct fs_device_d *fsdev = _dev->type_data;
	struct device_d *dev = fsdev->parent;
	char *f;

	f = strdup(filename);
	inode = cramfs_resolve (dev,
				 CRAMFS_GET_OFFSET (&(priv->super.root)) << 2,
				 CRAMFS_24 (priv->super.root.size), 0,
				 strtok (f, "/"));

	free(f);

	if (!inode)
		return -ENOENT;

	file->inode = inode;
	file->size = inode->size;

	return 0;
}

static int cramfs_close(struct device_d *dev, FILE *file)
{
	free(file->inode);
	return 0;
}

static int cramfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct cramfs_priv *priv = _dev->priv;
	struct fs_device_d *fsdev = _dev->type_data;
	struct device_d *dev = fsdev->parent;
	struct cramfs_inode *inode = (struct cramfs_inode *)f->inode;
	unsigned int blocknr;
	int outsize = 0;
	unsigned long *block_ptrs = (unsigned long *)
		(dev->map_base + (CRAMFS_GET_OFFSET (inode) << 2));
	int ofs = f->pos % 4096;

	if (f->pos + size > inode->size)
		size = inode->size - f->pos;

	while (size) {
		unsigned long base;
		int copy;

		blocknr = (f->pos + outsize) >> 12;
		if (blocknr != priv->curr_block || priv->inode != inode) {
			if (blocknr)
				base = CRAMFS_32 (block_ptrs[blocknr - 1]);
			else
				base = (CRAMFS_GET_OFFSET(inode) + (((CRAMFS_24 (inode->size)) + 4095) >> 12)) << 2;

			priv->curr_block_len = cramfs_uncompress_block (priv->buf,
					(void *)(dev->map_base + base), 4096);

//			printf("READ blocknr: %d len %d\n",blocknr,priv->curr_block_len );
			if (priv->curr_block_len <= 0)
				break;

			priv->curr_block = blocknr;
			priv->inode = inode;
		}

		copy = min(priv->curr_block_len, size);

		memcpy(buf, priv->buf + ofs, copy);
		ofs = 0;

		outsize += copy;
		size -= copy;
		buf += copy;
	}

	return outsize;
}

static int cramfs_stat(struct device_d *_dev, const char *filename, struct stat *stat)
{
	struct cramfs_priv *priv = _dev->priv;
	struct fs_device_d *fsdev = _dev->type_data;
	struct device_d *dev = fsdev->parent;
	struct cramfs_inode *inode;
	char *f;

	f = strdup(filename);

	inode = cramfs_resolve (dev,
			 CRAMFS_GET_OFFSET (&(priv->super.root)) << 2,
			 CRAMFS_24 (priv->super.root.size), 1,
			 strtok (f, "/"));
	free(f);

	if (!inode)
		return -ENOENT;

	stat->st_mode = CRAMFS_16 (inode->mode);
	stat->st_size = CRAMFS_24 (inode->size);

	free(inode);

	return 0;
}
#if 0
static int cramfs_info (struct device_d *dev)
{
	if (cramfs_read_super (dev))
		return 0;

	printf ("size: 0x%x (%u)\n", super.size, super.size);

	if (super.flags != 0) {
		printf ("flags:\n");
		if (super.flags & CRAMFS_FLAG_FSID_VERSION_2)
			printf ("\tFSID version 2\n");
		if (super.flags & CRAMFS_FLAG_SORTED_DIRS)
			printf ("\tsorted dirs\n");
		if (super.flags & CRAMFS_FLAG_HOLES)
			printf ("\tholes\n");
		if (super.flags & CRAMFS_FLAG_SHIFTED_ROOT_OFFSET)
			printf ("\tshifted root offset\n");
	}

	printf ("fsid:\n\tcrc: 0x%x\n\tedition: 0x%x\n",
		super.fsid.crc, super.fsid.edition);
	printf ("name: %16s\n", super.name);

	return 1;
}
#endif

int cramfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev;
	struct cramfs_priv *priv;

	fsdev = dev->type_data;

	priv = xmalloc(sizeof(struct cramfs_priv));
	dev->priv = priv;

	if (cramfs_read_super (fsdev->parent, priv)) {
		printf("no valid cramfs found on %s\n",dev->id);
		free(priv);
		return -EINVAL;
	}

	priv->curr_block = -1;

	cramfs_uncompress_init ();
	return 0;
}

int cramfs_remove(struct device_d *dev)
{
	struct cramfs_priv *priv = dev->priv;

	cramfs_uncompress_exit();
	free(priv);

	return 0;
}

static struct fs_driver_d cramfs_driver = {
	.type		= FS_TYPE_CRAMFS,
	.open		= cramfs_open,
	.close		= cramfs_close,
	.read		= cramfs_read,
	.opendir	= cramfs_opendir,
	.readdir	= cramfs_readdir,
	.closedir	= cramfs_closedir,
	.stat		= cramfs_stat,
	.drv = {
		.type = DEVICE_TYPE_FS,
		.probe = cramfs_probe,
		.remove = cramfs_remove,
		.name = "cramfs",
		.type_data = &cramfs_driver,
	}
};

int cramfs_init(void)
{
	return register_driver(&cramfs_driver.drv);
}

device_initcall(cramfs_init);

