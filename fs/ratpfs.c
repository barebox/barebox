/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#define pr_fmt(fmt) "barebox-ratpfs: " fmt

#include <common.h>
#include <command.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <errno.h>
#include <linux/stat.h>
#include <asm/unaligned.h>
#include <ratp_bb.h>

#define RATPFS_TYPE_MOUNT_CALL       1
#define RATPFS_TYPE_MOUNT_RETURN     2
#define RATPFS_TYPE_READDIR_CALL     3
#define RATPFS_TYPE_READDIR_RETURN   4
#define RATPFS_TYPE_STAT_CALL        5
#define RATPFS_TYPE_STAT_RETURN      6
#define RATPFS_TYPE_OPEN_CALL        7
#define RATPFS_TYPE_OPEN_RETURN      8
#define RATPFS_TYPE_READ_CALL        9
#define RATPFS_TYPE_READ_RETURN     10
#define RATPFS_TYPE_WRITE_CALL      11
#define RATPFS_TYPE_WRITE_RETURN    12
#define RATPFS_TYPE_CLOSE_CALL      13
#define RATPFS_TYPE_CLOSE_RETURN    14
#define RATPFS_TYPE_TRUNCATE_CALL   15
#define RATPFS_TYPE_TRUNCATE_RETURN 16

struct ratpfs_file {
	uint32_t handle;
};

struct ratpfs_dir {
	char *entries;
	int len, off;
	DIR dir;
};

static int ratpfs_create(struct device_d __always_unused *dev,
			const char __always_unused *pathname,
			mode_t __always_unused mode)
{
	pr_debug("%s\n", __func__);

	return 0;
}

static int ratpfs_mkdir(struct device_d __always_unused *dev,
		       const char __always_unused *pathname)
{
	pr_debug("%s\n", __func__);

	return -ENOSYS;
}

static int ratpfs_rm(struct device_d __always_unused *dev,
		    const char *pathname)
{
	pr_debug("%s\n", __func__);

	/* Get rid of leading '/' */
	pathname = &pathname[1];

	return 0;
}

static int ratpfs_truncate(struct device_d __always_unused *dev,
			  FILE *f, ulong size)
{
	int len_tx = 1 /* type */
		+ 4 /* handle */
		+ 4 /* size */;
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx)+len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	struct ratpfs_file *rfile = f->priv;
	int ret;

	pr_debug("%s: len_tx=%i handle=%i size=%i\n", __func__,
		 len_tx, rfile->handle, (int)size);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_TRUNCATE_CALL;
	put_unaligned_be32(rfile->handle, &pkt_tx->data[1]);
	put_unaligned_be32(size, &pkt_tx->data[5]);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);

	if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_TRUNCATE_RETURN) {
		pr_err("invalid truncate response\n");
		ret = -EIO;
		goto out;
	}

out:
	free(pkt_rx);
	return ret;
}

static int ratpfs_open(struct device_d __always_unused *dev,
		      FILE *file, const char *filename)
{
	int len_name = strlen(filename);
	int len_tx = 1 /* type */
		+ 4 /* flags */
		+ len_name /* path */;
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx) + len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	struct ratpfs_file *rfile = xzalloc(sizeof(*rfile));
	int ret;

	pr_debug("%s: len_tx=%i filename='%s'\n", __func__, len_tx, filename);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_OPEN_CALL;
	put_unaligned_be32(file->flags, &pkt_tx->data[1]);
	memcpy(&pkt_tx->data[5], filename, len_name);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret) {
		ret = -EIO;
		goto err;
	}

	pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);
	if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_OPEN_RETURN) {
		pr_err("invalid open response\n");
		ret = -EIO;
		goto err;
	}
	rfile->handle = get_unaligned_be32(&pkt_rx->data[1]);
	if (rfile->handle == 0) {
		ret = -get_unaligned_be32(&pkt_rx->data[5]); /* errno */
		goto err;
	}
	file->priv = rfile;
	file->size = get_unaligned_be32(&pkt_rx->data[5]);

	goto out;

err:
	file->priv = NULL;
	free(rfile);
out:
	free(pkt_rx);
	return ret;
}

static int ratpfs_close(struct device_d __always_unused *dev,
			FILE *f)
{
	int len_tx = 1 /* type */
		+ 4 /* handle */;
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx) + len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	struct ratpfs_file *rfile = f->priv;
	int ret;

	pr_debug("%s: len_tx=%i handle=%i\n", __func__,
		 len_tx, rfile->handle);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_CLOSE_CALL;
	put_unaligned_be32(rfile->handle, &pkt_tx->data[1]);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);

	if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_CLOSE_RETURN) {
		pr_err("invalid close response\n");
		goto out;
	}

out:
	free(pkt_rx);
	return ret;
}

static int ratpfs_write(struct device_d __always_unused *dev,
			FILE *f, const void *buf, size_t orig_size)
{
	int size = min((int)orig_size, 4096);
	int len_tx = 1 /* type */
		+ 4 /* handle */
		+ 4 /* pos */
		+ size /* data */;
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx) + len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	struct ratpfs_file *rfile = f->priv;
	int ret;

	pr_debug("%s: len_tx=%i handle=%i pos=%i size=%i\n", __func__,
		 len_tx, rfile->handle, (int)f->pos, size);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_WRITE_CALL;
	put_unaligned_be32(rfile->handle, &pkt_tx->data[1]);
	put_unaligned_be32(f->pos, &pkt_tx->data[5]);
	memcpy(&pkt_tx->data[9], buf, size);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);

	if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_WRITE_RETURN) {
		pr_err("invalid write response\n");
		ret = -EIO;
		goto out;
	}

	ret = size;
out:
	free(pkt_rx);

	return ret;
}

static int ratpfs_read(struct device_d __always_unused *dev,
		       FILE *f, void *buf, size_t orig_size)
{
	int size = min((int)orig_size, 4096);
	int len_tx = 1 /* type */
		+ 4 /* handle */
		+ 4 /* pos */
		+ 4 /* size */;
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx) + len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	struct ratpfs_file *rfile = f->priv;
	int ret;

	pr_debug("%s: len_tx=%i handle=%i pos=%i size=%i\n", __func__,
		 len_tx, rfile->handle, (int)f->pos, size);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_READ_CALL;
	put_unaligned_be32(rfile->handle, &pkt_tx->data[1]);
	put_unaligned_be32(f->pos, &pkt_tx->data[5]);
	put_unaligned_be32(size, &pkt_tx->data[9]);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);
	if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_READ_RETURN) {
		pr_err("invalid read response\n");
		ret = -EIO;
		goto out;
	}
	size = pkt_rx->len - 1;
	memcpy(buf, &pkt_rx->data[1], size);
	ret = size;

out:
	free(pkt_rx);
	return ret;
}

static loff_t ratpfs_lseek(struct device_d __always_unused *dev,
			  FILE *f, loff_t pos)
{
	pr_debug("%s\n", __func__);
	f->pos = pos;
	return f->pos;
}

static DIR* ratpfs_opendir(struct device_d __always_unused *dev,
			  const char *pathname)
{
	int len_name = strlen(pathname);
	int len_tx = 1 /* type */
		+ len_name /* path */;
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx)+len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	struct ratpfs_dir *rdir = xzalloc(sizeof(*rdir));
	int ret;

	pr_debug("%s: len_tx=%i pathname='%s'\n", __func__, len_tx, pathname);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_READDIR_CALL;
	memcpy(&pkt_tx->data[1], pathname, len_name);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (!ret) {
		pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);
		if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_READDIR_RETURN) {
			pr_err("invalid readdir response\n");
			free(pkt_rx);
			return NULL;
		}
		rdir->len = pkt_rx->len - 1;
		rdir->entries = xmemdup(&pkt_rx->data[1], rdir->len);
		free(pkt_rx);
		return &rdir->dir;
	} else {
		return NULL;
	}
}

static struct dirent *ratpfs_readdir(struct device_d *dev, DIR *dir)
{
	struct ratpfs_dir *rdir = container_of(dir, struct ratpfs_dir, dir);
	int i;

	pr_debug("%s\n", __func__);

	if (rdir->len <= rdir->off)
		return NULL;

	for (i = 0; rdir->off < rdir->len; rdir->off++, i++) {
		dir->d.d_name[i] = rdir->entries[rdir->off];
		if (dir->d.d_name[i] == 0)
			break;
	}
	rdir->off++;

	return &dir->d;
}

static int ratpfs_closedir(struct device_d *dev, DIR *dir)
{
	struct ratpfs_dir *rdir = container_of(dir, struct ratpfs_dir, dir);

	pr_debug("%s\n", __func__);

	free(rdir->entries);
	free(rdir);

	return 0;
}

static int ratpfs_stat(struct device_d __always_unused *dev,
		      const char *filename, struct stat *s)
{
	int len_name = strlen(filename);
	int len_tx = 1 /* type */
		+ len_name; /* path */
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx) + len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	int ret;

	pr_debug("%s: len_tx=%i filename='%s'\n", __func__, len_tx, filename);

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_STAT_CALL;
	memcpy(&pkt_tx->data[1], filename, len_name);

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	pr_debug("%s: len_rx=%i\n", __func__, pkt_rx->len);
	if (pkt_rx->len < 6 || pkt_rx->data[0] != RATPFS_TYPE_STAT_RETURN) {
		pr_err("invalid stat response\n");
		goto out;
	}
	switch (pkt_rx->data[1]) {
	case 0:
		ret = -ENOENT;
		break;
	case 1:
		s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		break;
	case 2:
		s->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		break;
	}
	s->st_size = get_unaligned_be32(&pkt_rx->data[2]);

out:
	free(pkt_rx);
	return ret;
}

static int ratpfs_probe(struct device_d *dev)
{
	int len_tx = 1; /* type */
	struct ratp_bb_pkt *pkt_tx = xzalloc(sizeof(*pkt_tx) + len_tx);
	struct ratp_bb_pkt *pkt_rx = NULL;
	int ret;
	struct fs_device_d *fsdev = dev_to_fs_device(dev);

	pr_debug("%s\n", __func__);

	ret = barebox_ratp_fs_mount(fsdev->path);
	if (ret)
		return ret;

	pkt_tx->len = len_tx;
	pkt_tx->data[0] = RATPFS_TYPE_MOUNT_CALL;

	ret = barebox_ratp_fs_call(pkt_tx, &pkt_rx);
	if (ret)
		goto out;

	if (pkt_rx->len < 1 || pkt_rx->data[0] != RATPFS_TYPE_MOUNT_RETURN) {
		pr_err("invalid mount response\n");
		ret = -EINVAL;
		goto out;
	}

out:
	free(pkt_rx);

	if (ret)
		barebox_ratp_fs_mount(NULL);

	return ret;
}

static void ratpfs_remove(struct device_d __always_unused *dev)
{
	pr_debug("%s\n", __func__);

	barebox_ratp_fs_mount(NULL);
}

static struct fs_driver_d ratpfs_driver = {
	.open      = ratpfs_open,
	.close     = ratpfs_close,
	.read      = ratpfs_read,
	.lseek     = ratpfs_lseek,
	.opendir   = ratpfs_opendir,
	.readdir   = ratpfs_readdir,
	.closedir  = ratpfs_closedir,
	.stat      = ratpfs_stat,
	.create    = ratpfs_create,
	.unlink    = ratpfs_rm,
	.mkdir     = ratpfs_mkdir,
	.rmdir     = ratpfs_rm,
	.write     = ratpfs_write,
	.truncate  = ratpfs_truncate,
	.flags     = FS_DRIVER_NO_DEV,
	.drv = {
		.probe  = ratpfs_probe,
		.remove = ratpfs_remove,
		.name = "ratpfs",
	}
};

static int ratpfs_init(void)
{
	return register_fs_driver(&ratpfs_driver);
}
coredevice_initcall(ratpfs_init);