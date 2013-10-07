/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 *
 * (C) Copyright 2008-2010
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Artem Bityutskiy (Битюцкий Артём)
 *          Adrian Hunter
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <command.h>
#include <errno.h>
#include <linux/stat.h>
#include <linux/ctype.h>
#include <linux/zlib.h>
#include <xfuncs.h>
#include <fcntl.h>

#include "ubifs.h"

struct ubifs_priv {
	struct cdev *cdev;
	struct ubi_volume_desc *ubi;
	struct super_block *sb;
};

struct z_stream_s ubifs_zlib_stream;

/* compress.c */

static int ubifs_deflate_decompress(const u8 *src, unsigned int slen, u8 *dst,
		unsigned int *dlen)
{
	return deflate_decompress(&ubifs_zlib_stream, src, slen, dst, dlen);
}

/* All UBIFS compressors */
struct ubifs_compressor ubifs_compressors[UBIFS_COMPR_TYPES_CNT] = {
	[UBIFS_COMPR_NONE] = {
		.compr_type = UBIFS_COMPR_NONE,
		.name = "no compression",
		.capi_name = "",
		.decompress = NULL,
	},
	[UBIFS_COMPR_LZO] = {
		.compr_type = UBIFS_COMPR_LZO,
		.name = "LZO",
#ifdef CONFIG_LZO_DECOMPRESS
		.capi_name = "lzo",
		.decompress = lzo1x_decompress_safe,
#endif
	},
	[UBIFS_COMPR_ZLIB] = {
		.compr_type = UBIFS_COMPR_ZLIB,
		.name = "zlib",
#ifdef CONFIG_ZLIB
		.capi_name = "deflate",
		.decompress = ubifs_deflate_decompress,
#endif
	},
};

/**
 * ubifs_decompress - decompress data.
 * @in_buf: data to decompress
 * @in_len: length of the data to decompress
 * @out_buf: output buffer where decompressed data should
 * @out_len: output length is returned here
 * @compr_type: type of compression
 *
 * This function decompresses data from buffer @in_buf into buffer @out_buf.
 * The length of the uncompressed data is returned in @out_len. This functions
 * returns %0 on success or a negative error code on failure.
 */
static int ubifs_decompress(const void *in_buf, int in_len, void *out_buf,
		     int *out_len, int compr_type)
{
	int err;
	struct ubifs_compressor *compr;

	if (unlikely(compr_type < 0 || compr_type >= UBIFS_COMPR_TYPES_CNT)) {
		ubifs_err("invalid compression type %d", compr_type);
		return -EINVAL;
	}

	compr = &ubifs_compressors[compr_type];

	if (unlikely(!compr->capi_name)) {
		ubifs_err("%s compression is not compiled in", compr->name);
		return -EINVAL;
	}

	if (compr_type == UBIFS_COMPR_NONE) {
		memcpy(out_buf, in_buf, in_len);
		*out_len = in_len;
		return 0;
	}

	err = compr->decompress(in_buf, in_len, out_buf, (size_t *)out_len);
	if (err)
		ubifs_err("cannot decompress %d bytes, compressor %s, "
			  "error %d", in_len, compr->name, err);

	return err;
}

/*
 * ubifsls...
 */

static int ubifs_finddir(struct super_block *sb, char *dirname,
			 unsigned long root_inum, unsigned long *inum)
{
	int err;
	struct qstr nm;
	union ubifs_key key;
	struct ubifs_dent_node *dent;
	struct ubifs_info *c;
	struct file *file;
	struct dentry *dentry;
	struct inode *dir;
	int ret = 0;

	file = kzalloc(sizeof(struct file), 0);
	dentry = kzalloc(sizeof(struct dentry), 0);
	dir = kzalloc(sizeof(struct inode), 0);
	if (!file || !dentry || !dir) {
		printf("%s: Error, no memory for malloc!\n", __func__);
		err = -ENOMEM;
		goto out;
	}

	dir->i_sb = sb;
	file->f_path.dentry = dentry;
	file->f_path.dentry->d_parent = dentry;
	file->f_path.dentry->d_inode = dir;
	file->f_path.dentry->d_inode->i_ino = root_inum;
	c = sb->s_fs_info;

	dbg_gen("dir ino %lu, f_pos %#llx", dir->i_ino, file->f_pos);

	/* Find the first entry in TNC and save it */
	lowest_dent_key(c, &key, dir->i_ino);
	nm.name = NULL;
	dent = ubifs_tnc_next_ent(c, &key, &nm);
	if (IS_ERR(dent)) {
		err = PTR_ERR(dent);
		goto out;
	}

	file->f_pos = key_hash_flash(c, &dent->key);
	file->private_data = dent;

	while (1) {
		dbg_gen("feed '%s', ino %llu, new f_pos %#x",
			dent->name, (unsigned long long)le64_to_cpu(dent->inum),
			key_hash_flash(c, &dent->key));
		ubifs_assert(le64_to_cpu(dent->ch.sqnum) > ubifs_inode(dir)->creat_sqnum);

		nm.len = le16_to_cpu(dent->nlen);
		if ((strncmp(dirname, (char *)dent->name, nm.len) == 0) &&
		    (strlen(dirname) == nm.len)) {
			*inum = le64_to_cpu(dent->inum);
			ret = 1;
			goto out_free;
		}

		/* Switch to the next entry */
		key_read(c, &dent->key, &key);
		nm.name = (char *)dent->name;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}

		kfree(file->private_data);
		file->f_pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
		cond_resched();
	}

out:
	if (err != -ENOENT)
		ubifs_err("cannot find next direntry, error %d", err);

out_free:
	kfree(file->private_data);
	free(file);
	free(dentry);
	free(dir);

	return ret;
}

static struct inode *ubifs_findfile(struct super_block *sb, const char *filename)
{
	int ret;
	char *next;
	char fpath[128];
	char *name = fpath;
	unsigned long root_inum = 1;
	unsigned long inum;
	struct inode *inode = 0;

	strcpy(fpath, filename);

	/* Remove all leading slashes */
	while (*name == '/')
		name++;

	/*
	 * Handle root-direcoty ('/')
	 */
	inum = root_inum;
	if (!name || *name == '\0')
		return ubifs_iget(sb, 1);

	for (;;) {
		struct ubifs_inode *ui;

		/* Extract the actual part from the pathname.  */
		next = strchr(name, '/');
		if (next) {
			/* Remove all leading slashes.  */
			while (*next == '/')
				*(next++) = '\0';
		}
		ret = ubifs_finddir(sb, name, root_inum, &inum);
		if (!ret)
			break;

		inode = ubifs_iget(sb, inum);
		if (!inode)
			break;

		ui = ubifs_inode(inode);

		/*
		 * Check if directory with this name exists
		 */

		/* Found the node!  */
		if (!next || *next == '\0')
			return inode;

		root_inum = inum;
		name = next;

		ubifs_iput(inode);
	}

	return NULL;
}

/*
 * ubifsload...
 */

/* file.c */

static int read_block(struct inode *inode, void *addr, unsigned int block,
		      struct ubifs_data_node *dn)
{
	struct ubifs_info *c = inode->i_sb->s_fs_info;
	int err, len, out_len;
	union ubifs_key key;
	unsigned int dlen;

	data_key_init(c, &key, inode->i_ino, block);
	err = ubifs_tnc_lookup(c, &key, dn);
	if (err) {
		if (err == -ENOENT)
			/* Not found, so it must be a hole */
			memset(addr, 0, UBIFS_BLOCK_SIZE);
		return err;
	}

	ubifs_assert(le64_to_cpu(dn->ch.sqnum) > ubifs_inode(inode)->creat_sqnum);

	len = le32_to_cpu(dn->size);
	if (len <= 0 || len > UBIFS_BLOCK_SIZE)
		goto dump;

	dlen = le32_to_cpu(dn->ch.len) - UBIFS_DATA_NODE_SZ;
	out_len = UBIFS_BLOCK_SIZE;
	err = ubifs_decompress(&dn->data, dlen, addr, &out_len,
			       le16_to_cpu(dn->compr_type));
	if (err || len != out_len)
		goto dump;

	/*
	 * Data length can be less than a full block, even for blocks that are
	 * not the last in the file (e.g., as a result of making a hole and
	 * appending data). Ensure that the remainder is zeroed out.
	 */
	if (len < UBIFS_BLOCK_SIZE)
		memset(addr + len, 0, UBIFS_BLOCK_SIZE - len);

	return 0;

dump:
	ubifs_err("bad data node (block %u, inode %lu)",
		  block, inode->i_ino);
	dbg_dump_node(c, dn);
	return -EINVAL;
}

struct ubifs_file {
	struct inode *inode;
	void *buf;
	unsigned int block;
	struct ubifs_data_node *dn;
};

static int ubifs_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct ubifs_priv *priv = dev->priv;
	struct inode *inode;
	struct ubifs_file *uf;

	inode = ubifs_findfile(priv->sb, filename);
	if (!inode)
		return -ENOENT;

	uf = xzalloc(sizeof(*uf));

	uf->inode = inode;
	uf->buf = xmalloc(UBIFS_BLOCK_SIZE);
	uf->dn = xzalloc(UBIFS_MAX_DATA_NODE_SZ);
	uf->block = -1;

	file->size = inode->i_size;
	file->inode = uf;

	return 0;
}

static int ubifs_close(struct device_d *dev, FILE *f)
{
	struct ubifs_file *uf = f->inode;
	struct inode *inode = uf->inode;

	ubifs_iput(inode);

	free(uf->buf);
	free(uf->dn);
	free(uf);

	return 0;
}

static int ubifs_get_block(struct ubifs_file *uf, unsigned int pos)
{
	int ret;
	unsigned int block = pos / UBIFS_BLOCK_SIZE;

	if (block != uf->block) {
		ret = read_block(uf->inode, uf->buf, block, uf->dn);
		if (ret)
			return ret;
		uf->block = block;
	}

	return 0;
}

static int ubifs_read(struct device_d *_dev, FILE *f, void *buf, size_t insize)
{
	struct ubifs_file *uf = f->inode;
	unsigned int pos = f->pos;
	unsigned int ofs;
	unsigned int now;
	unsigned int size = insize;
	int ret;

	/* Read till end of current block */
	ofs = f->pos % UBIFS_BLOCK_SIZE;
	if (ofs) {
		ret = ubifs_get_block(uf, pos);
		if (ret)
			return ret;

		now = min(size, UBIFS_BLOCK_SIZE - ofs);

		memcpy(buf, uf->buf + ofs, now);
		size -= now;
		pos += now;
		buf += now;
	}

	/* Do full blocks */
	while (size >= UBIFS_BLOCK_SIZE) {
		ret = ubifs_get_block(uf, pos);
		if (ret)
			return ret;

		memcpy(buf, uf->buf, UBIFS_BLOCK_SIZE);
		size -= UBIFS_BLOCK_SIZE;
		pos += UBIFS_BLOCK_SIZE;
		buf += UBIFS_BLOCK_SIZE;
	}

	/* And the rest */
	if (size) {
		ret = ubifs_get_block(uf, pos);
		if (ret)
			return ret;
		memcpy(buf, uf->buf, size);
	}

	return insize;
}

static loff_t ubifs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;

	return pos;
}

struct ubifs_dir {
	struct file file;
	struct dentry dentry;
	struct inode inode;
	DIR dir;
	union ubifs_key key;
	struct ubifs_dent_node *dent;
	struct ubifs_priv *priv;
	struct qstr nm;
};

static DIR *ubifs_opendir(struct device_d *dev, const char *pathname)
{
	struct ubifs_priv *priv = dev->priv;
	struct ubifs_dir *dir;
	struct file *file;
	struct dentry *dentry;
	struct inode *inode;
	unsigned long inum;
	struct ubifs_info *c = priv->sb->s_fs_info;

	inode = ubifs_findfile(priv->sb, pathname);
	if (!inode)
		return NULL;

	inum = inode->i_ino;

	ubifs_iput(inode);

	dir = xzalloc(sizeof(*dir));

	dir->priv = priv;

	file = &dir->file;
	dentry = &dir->dentry;
	inode = &dir->inode;

	inode->i_sb = priv->sb;
	file->f_path.dentry = dentry;
	file->f_path.dentry->d_parent = dentry;
	file->f_path.dentry->d_inode = inode;
	file->f_path.dentry->d_inode->i_ino = inum;
	file->f_pos = 1;

	/* Find the first entry in TNC and save it */
	lowest_dent_key(c, &dir->key, inode->i_ino);

	return &dir->dir;
}

static struct dirent *ubifs_readdir(struct device_d *dev, DIR *_dir)
{
	struct ubifs_dir *dir = container_of(_dir, struct ubifs_dir, dir);
	struct ubifs_info *c = dir->priv->sb->s_fs_info;
	struct ubifs_dent_node *dent;
	struct qstr *nm = &dir->nm;
	struct file *file = &dir->file;

	dent = ubifs_tnc_next_ent(c, &dir->key, nm);
	if (IS_ERR(dent))
		return NULL;

	debug("feed '%s', ino %llu, new f_pos %#x\n",
		dent->name, (unsigned long long)le64_to_cpu(dent->inum),
		key_hash_flash(c, &dent->key));

	ubifs_assert(le64_to_cpu(dent->ch.sqnum) > ubifs_inode(&dir->inode)->creat_sqnum);

	key_read(c, &dent->key, &dir->key);
	file->f_pos = key_hash_flash(c, &dent->key);
	file->private_data = dent;

	nm->len = le16_to_cpu(dent->nlen);
	nm->name = dent->name;

	strcpy(_dir->d.d_name, dent->name);

	free(dir->dent);
	dir->dent = dent;

	return &_dir->d;
}

static int ubifs_closedir(struct device_d *dev, DIR *_dir)
{
	struct ubifs_dir *dir = container_of(_dir, struct ubifs_dir, dir);

	free(dir->dent);
	free(dir);

	return 0;
}

static int ubifs_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct ubifs_priv *priv = dev->priv;
	struct inode *inode;

	inode = ubifs_findfile(priv->sb, filename);
	if (!inode)
		return -ENOENT;

	s->st_size = inode->i_size;
	s->st_mode = inode->i_mode;

	ubifs_iput(inode);

	return 0;
}

static int ubifs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct ubifs_priv *priv = xzalloc(sizeof(struct ubifs_priv));
	int ret;

	dev->priv = priv;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_free;

	priv->cdev = fsdev->cdev;

	priv->ubi = ubi_open_volume_cdev(priv->cdev, UBI_READONLY);
	if (IS_ERR(priv->ubi)) {
		dev_err(dev, "failed to open ubi volume: %s\n",
				strerror(-PTR_ERR(priv->ubi)));
		ret = PTR_ERR(priv->ubi);
		goto err_free;
	}

	priv->sb = ubifs_get_super(priv->ubi, 0);
	if (IS_ERR(priv->sb)) {
		ret = PTR_ERR(priv->sb);
		goto err;
	}

	return 0;
err:
	ubi_close_volume(priv->ubi);
err_free:
	free(priv);
	return ret;
}

static void ubifs_remove(struct device_d *dev)
{
	struct ubifs_priv *priv = dev->priv;
	struct super_block *sb = priv->sb;
	struct ubifs_info *c = sb->s_fs_info;

	ubifs_umount(c);
	ubi_close_volume(priv->ubi);

	free(c);
	free(sb);

	free(priv);
}

static struct fs_driver_d ubifs_driver = {
	.open      = ubifs_open,
	.close     = ubifs_close,
	.read      = ubifs_read,
	.lseek     = ubifs_lseek,
	.opendir   = ubifs_opendir,
	.readdir   = ubifs_readdir,
	.closedir  = ubifs_closedir,
	.stat      = ubifs_stat,
	.type = filetype_ubifs,
	.flags     = 0,
	.drv = {
		.probe  = ubifs_probe,
		.remove = ubifs_remove,
		.name = "ubifs",
	}
};

static int zlib_decomp_init(void)
{
	struct z_stream_s *stream = &ubifs_zlib_stream;
	int ret;

	stream->workspace = xzalloc(zlib_inflate_workspacesize());

	ret = zlib_inflateInit2(stream, -MAX_WBITS);
	if (ret != Z_OK) {
		free(stream->workspace);
		return -EINVAL;
	}

	return 0;
}

static int ubifs_init(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_ZLIB)) {
		ret = zlib_decomp_init();
		if (ret)
			return ret;
	}

	return register_fs_driver(&ubifs_driver);
}

coredevice_initcall(ubifs_init);
