/*
 * cramfs.c
 *
 * Copyright (C) 1999 Linus Torvalds
 *
 * Copyright (C) 2000-2002 Transmeta Corporation
 *
 * Copyright (C) 2003 Kai-Uwe Bloem,
 * Auerswald GmbH & Co KG, <linux-development@auerswald.de>
 * - adapted from the www.tuxbox.org barebox tree, added "ls" command
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
#include <cramfs/cramfs_fs.h>

/* These two macros may change in future, to provide better st_ino
   semantics. */
#define CRAMINO(x)	(CRAMFS_GET_OFFSET(x) ? CRAMFS_GET_OFFSET(x)<<2 : 1)
#define OFFSET(x)	((x)->i_ino)

struct cramfs_priv {
	struct cramfs_super super;
	int curr_base;
	char buf[4096];
	size_t curr_block_len;
	struct cdev *cdev;
};

struct cramfs_inode_info {
	struct cramfs_inode inode;
	unsigned long *block_ptrs;
};

static int cramfs_read_super(struct cramfs_priv *priv)
{
	unsigned long root_offset;
	struct cramfs_super *super = &priv->super;
	struct cdev *cdev = priv->cdev;

	if (cdev_read(cdev, super, sizeof (struct cramfs_super), 0, 0) < sizeof (struct cramfs_super)) {
		printf("read superblock failed\n");
		return -EINVAL;
	}

	/* Do sanity checks on the superblock */
	if (super->magic != CRAMFS_32 (CRAMFS_MAGIC)) {
		/* check at 512 byte offset */
		if (cdev_read(cdev, super, sizeof (struct cramfs_super), 512, 0) < sizeof (struct cramfs_super)) {
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

static struct cramfs_inode_info *cramfs_get_inode(struct cramfs_priv *priv, unsigned long offset)
{
	struct cramfs_inode_info *inodei = xmalloc(sizeof(*inodei));

	if (cdev_read(priv->cdev, &inodei->inode, sizeof(struct cramfs_inode), offset, 0) < 0) {
		free(inodei);
		return NULL;
	}

	return inodei;
}

static struct cramfs_inode_info *cramfs_resolve (struct cramfs_priv *priv, unsigned long offset,
				     unsigned long size, int raw,
				     char *filename)
{
	unsigned long inodeoffset = 0, nextoffset;
	struct cramfs_inode_info *inodei = NULL, *ret;
	char *name = xmalloc(256);

	while (inodeoffset < size) {
		int namelen;
		inodei = cramfs_get_inode(priv, offset + inodeoffset);

		/*
		 * Namelengths on disk are shifted by two
		 * and the name padded out to 4-byte boundaries
		 * with zeroes.
		 */
		namelen = CRAMFS_GET_NAMELEN (&inodei->inode) << 2;
		cdev_read(priv->cdev, name, namelen, offset + inodeoffset + sizeof (struct cramfs_inode), 0);

		nextoffset =
			inodeoffset + sizeof (struct cramfs_inode) + namelen;

		if (!strncmp (filename, name, namelen)) {
			char *p = strtok (NULL, "/");

			if (raw && (p == NULL || *p == '\0'))
				goto out1;

			if (S_ISDIR (CRAMFS_16 (inodei->inode.mode))) {
				ret = cramfs_resolve(priv,
					CRAMFS_GET_OFFSET(&inodei->inode) << 2,
					CRAMFS_24 (inodei->inode.size),
					raw, p);
				goto out;
			} else if (S_ISREG (CRAMFS_16 (inodei->inode.mode))) {
				goto out1;
			} else {
				printf ("%*.*s: unsupported file type (%x)\n",
					namelen, namelen, name,
					CRAMFS_16 (inodei->inode.mode));
				ret = NULL;
				goto out;
			}
		}

		free(inodei);
		inodeoffset = nextoffset;
	}

	free(name);
	return NULL;

out1:
	ret = cramfs_get_inode(priv, offset + inodeoffset);
out:
	free(inodei);
	free(name);
	return ret;
}

static int cramfs_fill_dirent (struct cramfs_priv *priv, unsigned long offset, struct dirent *d)
{
	struct cramfs_inode_info *inodei = cramfs_get_inode(priv, offset);
	int namelen;

	if (!inodei)
		return -EINVAL;

	memset(d->d_name, 0, 256);

	/*
	 * Namelengths on disk are shifted by two
	 * and the name padded out to 4-byte boundaries
	 * with zeroes.
	 */

	namelen = CRAMFS_GET_NAMELEN (&inodei->inode) << 2;
	cdev_read(priv->cdev, d->d_name, namelen, offset + sizeof(struct cramfs_inode), 0);
	free(inodei);
	return namelen;
}

struct cramfs_dir {
	unsigned long offset, size;
	unsigned long inodeoffset;
	DIR dir;
};

static DIR* cramfs_opendir(struct device_d *_dev, const char *filename)
{
	struct cramfs_priv *priv = _dev->priv;
	char *f;

	struct cramfs_dir *dir = xzalloc(sizeof(struct cramfs_dir));
	dir->dir.priv = dir;

	if (strlen (filename) == 0 || !strcmp (filename, "/")) {
		/* Root directory. Use root inode in super block */
		dir->offset = CRAMFS_GET_OFFSET (&(priv->super.root)) << 2;
		dir->size = CRAMFS_24 (priv->super.root.size);
	} else {
		struct cramfs_inode_info *inodei;

		f = strdup(filename);
		/* Resolve the path */
		inodei = cramfs_resolve(priv,
					 CRAMFS_GET_OFFSET (&(priv->super.root)) <<
					 2, CRAMFS_24 (priv->super.root.size), 1,
					 strtok (f, "/"));
		free(f);
		if (!inodei)
			goto err_free;

		/* Resolving was successful. Examine the inode */
		if (!S_ISDIR (CRAMFS_16 (inodei->inode.mode))) {
			/* It's not a directory */
			free(inodei);
			goto err_free;
		}

		dir->offset = CRAMFS_GET_OFFSET (&inodei->inode) << 2;
		dir->size = CRAMFS_24 (inodei->inode.size);
		free(inodei);
	}

	return &dir->dir;

err_free:
	free(dir);
	return NULL;
}

static struct dirent* cramfs_readdir(struct device_d *_dev, DIR *_dir)
{
	struct cramfs_priv *priv = _dev->priv;
	struct cramfs_dir *dir = _dir->priv;
	unsigned long nextoffset;

	/* List the given directory */
	if (dir->inodeoffset < dir->size) {
		nextoffset = cramfs_fill_dirent (priv, dir->offset + dir->inodeoffset, &_dir->d);

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
	struct cramfs_inode_info *inodei;
	char *f;

	f = strdup(filename);
	inodei = cramfs_resolve (priv,
				 CRAMFS_GET_OFFSET (&(priv->super.root)) << 2,
				 CRAMFS_24 (priv->super.root.size), 0,
				 strtok (f, "/"));
	free(f);

	if (!inodei)
		return -ENOENT;

	file->inode = inodei;
	file->size = inodei->inode.size;

	inodei->block_ptrs = xzalloc(4096);
	cdev_read(priv->cdev, inodei->block_ptrs, 4096, CRAMFS_GET_OFFSET(&inodei->inode) << 2, 0);

	return 0;
}

static int cramfs_close(struct device_d *dev, FILE *file)
{
	struct cramfs_inode_info *inodei = file->inode;

	free(inodei->block_ptrs);
	free(inodei);
	
	return 0;
}

static int cramfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct cramfs_priv *priv = _dev->priv;
	struct cramfs_inode_info *inodei = f->inode;
	struct cramfs_inode *inode = &inodei->inode;
	unsigned int blocknr;
	int outsize = 0;
	unsigned long *block_ptrs = inodei->block_ptrs;
	int ofs = f->pos % 4096;
	static char cramfs_read_buf[4096];

	if (f->pos + size > inode->size)
		size = inode->size - f->pos;

	while (size) {
		unsigned long base;
		int copy;

		blocknr = (f->pos + outsize) >> 12;

		if (blocknr)
			base = CRAMFS_32 (block_ptrs[blocknr - 1]);
		else
			base = (CRAMFS_GET_OFFSET(inode) + (((CRAMFS_24 (inode->size)) + 4095) >> 12)) << 2;

		if (priv->curr_base < 0 || priv->curr_base != base) {

			cdev_read(priv->cdev, cramfs_read_buf, 4096, base, 0);
			priv->curr_block_len = cramfs_uncompress_block(priv->buf, 4096,
					cramfs_read_buf, 4096);
			if (priv->curr_block_len <= 0)
				break;

			priv->curr_base = base;
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

static loff_t cramfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;
	return f->pos;
}

static int cramfs_stat(struct device_d *_dev, const char *filename, struct stat *stat)
{
	struct cramfs_priv *priv = _dev->priv;
	struct cramfs_inode_info *inodei;
	struct cramfs_inode *inode;
	char *f;

	f = strdup(filename);

	inodei = cramfs_resolve (priv,
			 CRAMFS_GET_OFFSET (&(priv->super.root)) << 2,
			 CRAMFS_24 (priv->super.root.size), 1,
			 strtok (f, "/"));
	free(f);

	if (!inodei)
		return -ENOENT;

	inode = &inodei->inode;
	stat->st_mode = CRAMFS_16 (inode->mode);
	stat->st_size = CRAMFS_24 (inode->size);

	free(inodei);

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

static int cramfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev;
	struct cramfs_priv *priv;
	int ret;

	fsdev = dev_to_fs_device(dev);

	priv = xmalloc(sizeof(struct cramfs_priv));
	dev->priv = priv;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_out;

	priv->cdev = fsdev->cdev;

	if (cramfs_read_super(priv)) {
		dev_info(dev, "no valid cramfs found\n");
		ret =  -EINVAL;
	}

	priv->curr_base = -1;

	cramfs_uncompress_init ();
	return 0;

err_out:
	free(priv);

	return ret;
}

static void cramfs_remove(struct device_d *dev)
{
	struct cramfs_priv *priv = dev->priv;

	cramfs_uncompress_exit();
	free(priv);
}

static struct fs_driver_d cramfs_driver = {
	.open		= cramfs_open,
	.close		= cramfs_close,
	.read		= cramfs_read,
	.lseek		= cramfs_lseek,
	.opendir	= cramfs_opendir,
	.readdir	= cramfs_readdir,
	.closedir	= cramfs_closedir,
	.stat		= cramfs_stat,
	.drv = {
		.probe = cramfs_probe,
		.remove = cramfs_remove,
		.name = "cramfs",
	}
};

static int cramfs_init(void)
{
	return register_fs_driver(&cramfs_driver);
}

device_initcall(cramfs_init);

