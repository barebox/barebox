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
#include <linux/magic.h>
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
	struct inode i_inode;
	struct cramfs_inode inode;
	unsigned long *block_ptrs;
};

static inline struct cramfs_inode_info* to_cramfs_inode_info(struct inode *inode)
{
	return container_of(inode, struct cramfs_inode_info, i_inode);
}

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

static int cramfs_read_file(struct inode *inode, unsigned long offset,
			    void *buf, size_t size)
{
	struct cramfs_inode_info *info = to_cramfs_inode_info(inode);
	struct cramfs_inode *cramfs_inode = &info->inode;
	struct fs_device_d *fsdev = container_of(inode->i_sb, struct fs_device_d, sb);
	struct cramfs_priv *priv = fsdev->dev.priv;
	unsigned int blocknr;
	int outsize = 0;
	int ofs = offset % 4096;
	static char cramfs_read_buf[4096];

	while (size) {
		uint32_t base;
		int copy;

		blocknr = (offset + outsize) >> 12;
		if (blocknr)
			cdev_read(priv->cdev, &base, 4,
				  OFFSET(inode) + (blocknr - 1) * 4, 0);
		else
			base = (CRAMFS_GET_OFFSET(cramfs_inode) +
				(((CRAMFS_24 (cramfs_inode->size)) + 4095) >> 12)) << 2;

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

static int cramfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	return cramfs_read_file(f->f_inode, f->pos, buf, size);
}

static loff_t cramfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;
	return f->pos;
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

static const struct file_operations cramfs_dir_operations;
static const struct inode_operations cramfs_dir_inode_operations;
static const struct inode_operations cramfs_symlink_inode_operations;

static unsigned long cramino(const struct cramfs_inode *cino, unsigned int offset)
{
	if (!cino->offset)
		return offset + 1;
	if (!cino->size)
		return offset + 1;

	/*
	 * The file mode test fixes buggy mkcramfs implementations where
	 * cramfs_inode->offset is set to a non zero value for entries
	 * which did not contain data, like devices node and fifos.
	 */
	switch (cino->mode & S_IFMT) {
	case S_IFREG:
	case S_IFDIR:
	case S_IFLNK:
		return cino->offset << 2;
	default:
		break;
	}
	return offset + 1;
}

static struct inode *get_cramfs_inode(struct super_block *sb,
	const struct cramfs_inode *cramfs_inode, unsigned int offset)
{
	struct cramfs_inode_info *info;
	static struct timespec zerotime;
	struct inode *inode;

	inode = new_inode(sb);

	inode->i_ino = cramino(cramfs_inode, offset);

	info = to_cramfs_inode_info(inode);

	switch (cramfs_inode->mode & S_IFMT) {
	case S_IFREG:
		break;
	case S_IFDIR:
		inode->i_op = &cramfs_dir_inode_operations;
		inode->i_fop = &cramfs_dir_operations;
		break;
	case S_IFLNK:
		inode->i_op = &cramfs_symlink_inode_operations;
		break;
	default:
		return NULL;
	}

	info->inode = *cramfs_inode;

	inode->i_mode = cramfs_inode->mode;

	/* if the lower 2 bits are zero, the inode contains data */
	if (!(inode->i_ino & 3)) {
		inode->i_size = cramfs_inode->size;
		inode->i_blocks = (cramfs_inode->size - 1) / 512 + 1;
	}

	/* Struct copy intentional */
	inode->i_mtime = inode->i_atime = inode->i_ctime = zerotime;
	/* inode->i_nlink is left 1 - arguably wrong for directories,
	   but it's the best we can do without reading the directory
	   contents.  1 yields the right result in GNU find, even
	   without -noleaf option. */

	return inode;
}

static struct dentry *cramfs_lookup(struct inode *dir, struct dentry *dentry,
				    unsigned int flags)
{
	struct cramfs_inode *de;
	unsigned int offset = 0;
	struct inode *inode = NULL;
	struct fs_device_d *fsdev = container_of(dir->i_sb, struct fs_device_d, sb);
	struct cramfs_priv *priv = fsdev->dev.priv;

	de = xmalloc(sizeof(*de) + CRAMFS_MAXPATHLEN);

	while (offset < dir->i_size) {
		char *name;
		int namelen, retval;
		int dir_off = OFFSET(dir) + offset;

		cdev_read(priv->cdev, de, sizeof(*de) + CRAMFS_MAXPATHLEN, dir_off, 0);

		name = (char *)(de + 1);

		namelen = de->namelen << 2;
		offset += sizeof(*de) + namelen;

		/* Quick check that the name is roughly the right length */
		if (((dentry->d_name.len + 3) & ~3) != namelen)
			continue;

		for (;;) {
			if (!namelen) {
				inode = ERR_PTR(-EIO);
				goto out;
			}
			if (name[namelen-1])
				break;
			namelen--;
		}
		if (namelen != dentry->d_name.len)
			continue;
		retval = memcmp(dentry->d_name.name, name, namelen);
		if (retval > 0)
			continue;
		if (!retval) {
			inode = get_cramfs_inode(dir->i_sb, de, dir_off);
			break;
		}
	}
out:
	free(de);

	if (IS_ERR(inode))
		return ERR_CAST(inode);
	d_add(dentry, inode);

	return NULL;
}

static struct inode *cramfs_alloc_inode(struct super_block *sb)
{
	struct cramfs_inode_info *info;

	info = xzalloc(sizeof(*info));
 
	return &info->i_inode;
}

static int cramfs_iterate(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *dir = d_inode(dentry);
	struct fs_device_d *fsdev = container_of(dir->i_sb, struct fs_device_d, sb);
	struct cramfs_priv *priv = fsdev->dev.priv;
	char *buf;
	unsigned int offset;
	struct cramfs_inode *de;
	int ret;

	/* Offset within the thing. */
	if (ctx->pos >= dir->i_size)
		return 0;
	offset = ctx->pos;
	/* Directory entries are always 4-byte aligned */
	if (offset & 3)
		return -EINVAL;

	buf = xmalloc(CRAMFS_MAXPATHLEN);
	de = xmalloc(sizeof(*de) + CRAMFS_MAXPATHLEN);

	while (offset < dir->i_size) {
		unsigned long nextoffset;
		char *name;
		ino_t ino;
		umode_t mode;
		int namelen;

		cdev_read(priv->cdev, de, sizeof(*de) + CRAMFS_MAXPATHLEN,
			  OFFSET(dir) + offset, 0);
		name = (char *)(de + 1);

		/*
		 * Namelengths on disk are shifted by two
		 * and the name padded out to 4-byte boundaries
		 * with zeroes.
		 */
		namelen = de->namelen << 2;
		memcpy(buf, name, namelen);
		ino = cramino(de, OFFSET(dir) + offset);
		mode = de->mode;

		nextoffset = offset + sizeof(*de) + namelen;
		for (;;) {
			if (!namelen) {
				ret = -EIO;
				goto out;
			}
			if (buf[namelen - 1])
				break;
			namelen--;
		}

		dir_emit(ctx, buf, namelen, ino, mode >> 12);

		ctx->pos = offset = nextoffset;
	}
	ret = 0;
out:
	kfree(buf);
	free(de);
	return ret;
}

static const struct file_operations cramfs_dir_operations = {
	.iterate = cramfs_iterate,
};

static const struct inode_operations cramfs_dir_inode_operations =
{
	.lookup = cramfs_lookup,
};

static const char *cramfs_get_link(struct dentry *dentry, struct inode *inode)
{
	int ret;

	inode->i_link = xzalloc(inode->i_size + 1);

	ret = cramfs_read_file(inode, 0, inode->i_link, inode->i_size);
	if (ret < 0)
		return NULL;

	return inode->i_link;
}

static const struct inode_operations cramfs_symlink_inode_operations =
{
	.get_link = cramfs_get_link,
};

static const struct super_operations cramfs_ops = {
	.alloc_inode = cramfs_alloc_inode,
};

static int cramfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev;
	struct cramfs_priv *priv;
	int ret;
	struct super_block *sb;
	struct inode *root;

	fsdev = dev_to_fs_device(dev);
	sb = &fsdev->sb;

	priv = xmalloc(sizeof(struct cramfs_priv));
	dev->priv = priv;

	ret = fsdev_open_cdev(fsdev);
	if (ret) {
		dev_err(dev, "open cdev failed: %d\n", ret);
		goto err_out;
	}

	priv->cdev = fsdev->cdev;

	if (cramfs_read_super(priv)) {
		dev_info(dev, "no valid cramfs found\n");
		ret =  -EINVAL;
	}

	priv->curr_base = -1;

	cramfs_uncompress_init ();

	sb->s_op = &cramfs_ops;

	root = get_cramfs_inode(sb, &priv->super.root, 0);
	if (IS_ERR(root))
		return PTR_ERR(root);
	sb->s_root = d_make_root(root);

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
	.read		= cramfs_read,
	.lseek		= cramfs_lseek,
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
