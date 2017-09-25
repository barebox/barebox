/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <malloc.h>
#include <fs.h>
#include <fcntl.h>
#include <init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <mach/omap4_rom_usb.h>

#define OMAP4_USBBOOT_FS_MAGIC		0x5562464D
#define OMAP4_USBBOOT_FS_CMD_OPEN	0x46530000
#define OMAP4_USBBOOT_FS_CMD_CLOSE	0x46530001
#define OMAP4_USBBOOT_FS_CMD_READ	0x46530002
#define OMAP4_USBBOOT_FS_CMD_END	0x4653FFFF

struct file_priv {
	s32 id;
	u32 size;
};
/*
static int omap4_usbbootfs_create(
	struct device_d *dev, const char *pathname, mode_t mode)
{
	return -ENOSYS;
}

static int omap4_usbbootfs_unlink(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int omap4_usbbootfs_mkdir(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int omap4_usbbootfs_rmdir(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int omap4_usbbootfs_write(
	struct device_d *_dev, FILE *f, const void *inbuf, size_t size)
{
	return -ENOSYS;
}

static int omap4_usbbootfs_truncate(struct device_d *dev, FILE *f, ulong size)
{
	return -ENOSYS;
}
*/

static struct file_priv *omap4_usbbootfs_do_open(
	struct device_d *dev, int accmode, const char *filename)
{
	struct file_priv *priv;
	u32 data;

	if (accmode & O_ACCMODE)
		return ERR_PTR(-ENOSYS);

	priv = xzalloc(sizeof(*priv));

	data = OMAP4_USBBOOT_FS_MAGIC	; omap4_usbboot_write(&data, 4);
	data = OMAP4_USBBOOT_FS_CMD_OPEN; omap4_usbboot_write(&data, 4);
	omap4_usbboot_puts(filename);
	data = OMAP4_USBBOOT_FS_CMD_END	; omap4_usbboot_write(&data, 4);

	if (omap4_usbboot_read(&priv->id, 4) ||
		omap4_usbboot_read(&priv->size, 4)
	) {
		free(priv);
		return ERR_PTR(-EIO);
	}
	if (priv->id < 0) {
		free(priv);
		return ERR_PTR(-ENOENT);
	}

	return priv;
}

static int omap4_usbbootfs_open(
	struct device_d *dev, FILE *file, const char *filename)
{
	struct file_priv *priv;

	priv = omap4_usbbootfs_do_open(dev, file->flags, filename);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	file->priv = priv;
	file->size = priv->size;

	return 0;
}

static int omap4_usbbootfs_do_close(struct file_priv *priv)
{
	u32 data;
	data = OMAP4_USBBOOT_FS_MAGIC	; omap4_usbboot_write(&data, 4);
	data = OMAP4_USBBOOT_FS_CMD_CLOSE; omap4_usbboot_write(&data, 4);
	omap4_usbboot_write(&priv->id, 4);
	data = OMAP4_USBBOOT_FS_CMD_END	; omap4_usbboot_write(&data, 4);
	free(priv);
	return 0;
}

static int omap4_usbbootfs_close(struct device_d *dev, FILE *f)
{
	struct file_priv *priv = f->priv;
	return omap4_usbbootfs_do_close(priv);
}

static int omap4_usbbootfs_read(
	struct device_d *dev, FILE *f, void *buf, size_t size)
{
	struct file_priv *priv = f->priv;
	u32 data;

	if (size > priv->size - f->pos)
		size = priv->size - f->pos;
	if (!size)
		return 0;

	data = OMAP4_USBBOOT_FS_MAGIC	; omap4_usbboot_write(&data, 4);
	data = OMAP4_USBBOOT_FS_CMD_READ; omap4_usbboot_write(&data, 4);
	omap4_usbboot_write(&priv->id, 4);
	omap4_usbboot_write(&f->pos, 4);
	omap4_usbboot_write(&size, 4);
	data = OMAP4_USBBOOT_FS_CMD_END	; omap4_usbboot_write(&data, 4);

	if (omap4_usbboot_read(buf, size))
		return -EIO;

	return size;
}

static loff_t omap4_usbbootfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;
	return pos;
}

static DIR *omap4_usbbootfs_opendir(struct device_d *dev, const char *pathname)
{
	return NULL;
}

static int omap4_usbbootfs_stat(
	struct device_d *dev, const char *filename, struct stat *s)
{
	struct file_priv *priv;

	priv = omap4_usbbootfs_do_open(dev, O_RDONLY, filename);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	s->st_mode = S_IFREG |
				 S_IRUSR | S_IRGRP | S_IROTH |
				 S_IXUSR | S_IXGRP | S_IXOTH ;
	s->st_size = priv->size;

	omap4_usbbootfs_do_close(priv);

	return 0;
}

static int omap4_usbbootfs_probe(struct device_d *dev)
{
	return 0;
}
static void omap4_usbbootfs_remove(struct device_d *dev)
{
}

static struct fs_driver_d omap4_usbbootfs_driver = {
	.open    = omap4_usbbootfs_open,
	.close   = omap4_usbbootfs_close,
	.read    = omap4_usbbootfs_read,
	.lseek   = omap4_usbbootfs_lseek,
	.opendir = omap4_usbbootfs_opendir,
	.stat    = omap4_usbbootfs_stat,
/*
	.create	= omap4_usbbootfs_create,
	.unlink	= omap4_usbbootfs_unlink,
	.mkdir	= omap4_usbbootfs_mkdir,
	.rmdir	= omap4_usbbootfs_rmdir,
	.write	= omap4_usbbootfs_write,
	.truncate= omap4_usbbootfs_truncate,
*/
	.flags	 = 0,
	.drv = {
		.probe	= omap4_usbbootfs_probe,
		.remove	= omap4_usbbootfs_remove,
		.name	= "omap4_usbbootfs",
	}
};

static int omap4_usbbootfs_init(void)
{
	return register_fs_driver(&omap4_usbbootfs_driver);
}
coredevice_initcall(omap4_usbbootfs_init);
