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
#include <mach/omap/omap4_rom_usb.h>

#define OMAP4_USBBOOT_FS_MAGIC		0x5562464D
#define OMAP4_USBBOOT_FS_CMD_OPEN	0x46530000
#define OMAP4_USBBOOT_FS_CMD_CLOSE	0x46530001
#define OMAP4_USBBOOT_FS_CMD_READ	0x46530002
#define OMAP4_USBBOOT_FS_CMD_END	0x4653FFFF

struct file_priv {
	s32 id;
	u32 size;
};

static struct file_priv *omap4_usbbootfs_do_open(struct device *dev,
						 int accmode,
						 const char *filename)
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

static int omap4_usbbootfs_open(struct device *dev, struct file *file,
				const char *filename)
{
	struct file_priv *priv;

	priv = omap4_usbbootfs_do_open(dev, file->f_flags, filename);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	file->private_data = priv;
	file->f_size = priv->size;

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

static int omap4_usbbootfs_close(struct device *dev, struct file *f)
{
	struct file_priv *priv = f->private_data;
	return omap4_usbbootfs_do_close(priv);
}

static int omap4_usbbootfs_read(struct device *dev, struct file *f, void *buf,
				size_t size)
{
	struct file_priv *priv = f->private_data;
	u32 data;

	if (size > priv->size - f->f_pos)
		size = priv->size - f->f_pos;
	if (!size)
		return 0;

	data = OMAP4_USBBOOT_FS_MAGIC	; omap4_usbboot_write(&data, 4);
	data = OMAP4_USBBOOT_FS_CMD_READ; omap4_usbboot_write(&data, 4);
	omap4_usbboot_write(&priv->id, 4);
	omap4_usbboot_write(&f->f_pos, 4);
	omap4_usbboot_write(&size, 4);
	data = OMAP4_USBBOOT_FS_CMD_END	; omap4_usbboot_write(&data, 4);

	if (omap4_usbboot_read(buf, size))
		return -EIO;

	return size;
}

static DIR *omap4_usbbootfs_opendir(struct device *dev, const char *pathname)
{
	return NULL;
}

static int omap4_usbbootfs_stat(struct device *dev, const char *filename,
				struct stat *s)
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

static int omap4_usbbootfs_probe(struct device *dev)
{
	return omap4_usbboot_open();
}

static void omap4_usbbootfs_remove(struct device *dev)
{
}

static struct fs_driver omap4_usbbootfs_driver = {
	.open    = omap4_usbbootfs_open,
	.close   = omap4_usbbootfs_close,
	.read    = omap4_usbbootfs_read,
	.opendir = omap4_usbbootfs_opendir,
	.stat    = omap4_usbbootfs_stat,
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
