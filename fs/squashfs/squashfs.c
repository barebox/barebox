#include <common.h>
#include <malloc.h>
#include <driver.h>
#include <init.h>
#include <errno.h>
#include <fs.h>
#include <xfuncs.h>

#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/pagemap.h>
#include <linux/mtd/ubi.h>
#include <linux/mtd/mtd.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs_fs_i.h"
#include "squashfs.h"

struct ubi_volume_desc;

char *squashfs_devread(struct squashfs_sb_info *fs, int byte_offset,
		int byte_len)
{
	ssize_t size;
	char *buf;

	buf = malloc(byte_len);
	if (buf == NULL)
		return NULL;

	size = cdev_read(fs->cdev, buf, byte_len, byte_offset, 0);
	if (size < 0) {
		dev_err(fs->dev, "read error: %s\n",
				strerror(-size));
		return NULL;
	}

	return buf;
}

static void squashfs_set_rootarg(struct fs_device_d *fsdev)
{
	struct ubi_volume_desc *ubi_vol;
	struct ubi_volume_info vi = {};
	struct ubi_device_info di = {};
	struct mtd_info *mtd;
	char *str;

	if (!IS_ENABLED(CONFIG_MTD_UBI))
		return;

	ubi_vol = ubi_open_volume_cdev(fsdev->cdev, UBI_READONLY);

	if (IS_ERR(ubi_vol))
		return;

	ubi_get_volume_info(ubi_vol, &vi);
	ubi_get_device_info(vi.ubi_num, &di);
	mtd = di.mtd;

	str = basprintf("root=/dev/ubiblock%d_%d ubi.mtd=%s ubi.block=%d,%d rootfstype=squashfs",
			vi.ubi_num, vi.vol_id, mtd->cdev.partname, vi.ubi_num, vi.vol_id);

	fsdev_set_linux_rootarg(fsdev, str);

	free(str);
}

static struct inode *squashfs_alloc_inode(struct super_block *sb)
{
	struct squashfs_inode_info *node;

	node = xzalloc(sizeof(*node));

	return &node->vfs_inode;
}

static const struct super_operations squashfs_super_ops = {
        .alloc_inode = squashfs_alloc_inode,
};

static int squashfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev;
	int ret;
	struct super_block *sb;

	fsdev = dev_to_fs_device(dev);
	sb = &fsdev->sb;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_out;


	ret = squashfs_mount(fsdev, 0);
	if (ret) {
		dev_err(dev, "no valid squashfs found\n");
		goto err_out;
	}

	squashfs_set_rootarg(fsdev);

	sb->s_op = &squashfs_super_ops;

	return 0;

err_out:

	return ret;
}

static void squashfs_remove(struct device_d *dev)
{
	struct fs_device_d *fsdev;
	struct super_block *sb;

	fsdev = dev_to_fs_device(dev);
	sb = &fsdev->sb;

	squashfs_put_super(sb);
}

static int squashfs_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct inode *inode = file->f_inode;
	struct squashfs_page *page;
	int i;

	page = malloc(sizeof(struct squashfs_page));
	page->buf = calloc(32, sizeof(*page->buf));
	for (i = 0; i < 32; i++) {
		page->buf[i] = malloc(PAGE_CACHE_SIZE);
		if (page->buf[i] == NULL) {
			dev_err(dev, "error allocation read buffer\n");
			goto error;
		}
	}

	page->data_block = 0;
	page->idx = 0;
	page->real_page.inode = inode;
	file->size = inode->i_size;
	file->priv = page;

	return 0;

error:
	for (; i > 0; --i)
		free(page->buf[i]);

	free(page->buf);
	free(page);

	return -ENOMEM;
}

static int squashfs_close(struct device_d *dev, FILE *f)
{
	struct squashfs_page *page = f->priv;
	int i;

	for (i = 0; i < 32; i++)
		free(page->buf[i]);

	free(page->buf);
	free(page);

	return 0;
}

static int squashfs_read_buf(struct squashfs_page *page, int pos, void **buf)
{
	unsigned int data_block = pos / (32 * PAGE_CACHE_SIZE);
	unsigned int data_block_pos = pos % (32 * PAGE_CACHE_SIZE);
	unsigned int idx = data_block_pos / PAGE_CACHE_SIZE;

	if (data_block != page->data_block || page->idx == 0) {
		page->idx = 0;
		page->real_page.index = data_block * 32;
		squashfs_readpage(NULL, &page->real_page);
		page->data_block = data_block;
	}

	*buf = page->buf[idx];

	return 0;
}

static int squashfs_read(struct device_d *_dev, FILE *f, void *buf,
		size_t insize)
{
	unsigned int size = insize;
	unsigned int pos = f->pos;
	unsigned int ofs;
	unsigned int now;
	void *pagebuf;
	struct squashfs_page *page = f->priv;

	/* Read till end of current buffer page */
	ofs = pos % PAGE_CACHE_SIZE;
	if (ofs) {
		squashfs_read_buf(page, pos, &pagebuf);

		now = min(size, PAGE_CACHE_SIZE - ofs);
		memcpy(buf, pagebuf + ofs, now);

		size -= now;
		pos += now;
		buf += now;
	}

	/* Do full buffer pages */
	while (size >= PAGE_CACHE_SIZE) {
		squashfs_read_buf(page, pos, &pagebuf);

		memcpy(buf, pagebuf, PAGE_CACHE_SIZE);
		size -= PAGE_CACHE_SIZE;
		pos += PAGE_CACHE_SIZE;
		buf += PAGE_CACHE_SIZE;
	}

	/* And the rest */
	if (size) {
		squashfs_read_buf(page, pos, &pagebuf);
		memcpy(buf, pagebuf, size);
		size  = 0;
	}

	return insize;
}

static loff_t squashfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;

	return pos;
}

struct squashfs_dir {
	struct file file;
	struct dentry dentry;
	struct dentry root_dentry;
	struct inode inode;
	struct qstr nm;
	DIR dir;
	char d_name[256];
	char root_d_name[256];
};

static struct fs_driver_d squashfs_driver = {
	.open		= squashfs_open,
	.close		= squashfs_close,
	.read		= squashfs_read,
	.lseek		= squashfs_lseek,
	.type		= filetype_squashfs,
	.drv = {
		.probe = squashfs_probe,
		.remove = squashfs_remove,
		.name = "squashfs",
	}
};

static int squashfs_init(void)
{
	return register_fs_driver(&squashfs_driver);
}

device_initcall(squashfs_init);
