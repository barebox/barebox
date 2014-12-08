/*
 * efivars.c - EFI variable filesystem
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <string.h>
#include <errno.h>
#include <linux/stat.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <efi.h>
#include <wchar.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <mach/efi.h>
#include <mach/efi-device.h>

struct efivarfs_inode {
	char *name;
	struct list_head node;
};

struct efivarfs_dir {
	struct list_head *current;
	DIR dir;
};

struct efivarfs_priv {
	struct list_head inodes;
};

static int char_to_nibble(char c)
{
	int ret = tolower(c);

	return ret <= '9' ? ret - '0' : ret - 'a' + 10;
}

static int read_byte_str(const char *str, u8 *out)
{
	if (!isxdigit(*str) || !isxdigit(*(str + 1)))
		return -EINVAL;

	*out = (char_to_nibble(*str) << 4) | char_to_nibble(*(str + 1));

	return 0;
}

int efi_guid_parse(const char *str, efi_guid_t *guid)
{
	int i, ret;
	u8 idx[] = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };

	for (i = 0; i < 16; i++) {
		ret = read_byte_str(str, &guid->b[idx[i]]);
		if (ret)
			return ret;
		str += 2;

		switch (i) {
		case 3:
		case 5:
		case 7:
		case 9:
			if (*str != '-')
				return -EINVAL;
			str++;
			break;
		}
	}

	return 0;
}

static int efivarfs_parse_filename(const char *filename, efi_guid_t *vendor, s16 **name)
{
	int len, ret;
	const char *guidstr;
	s16 *varname;
	int i;

	if (*filename == '/')
		filename++;

	len = strlen(filename);

	if (len < sizeof("-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"))
		return -EINVAL;

	guidstr = filename + len - sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
	if (*guidstr != '-')
		return -EINVAL;

	guidstr++;

	ret = efi_guid_parse(guidstr, vendor);

	varname = xzalloc((guidstr - filename) * sizeof(s16));

	for (i = 0; i < guidstr - filename - 1; i++)
		varname[i] = filename[i];

	*name = varname;

	return 0;
}

struct efivars_file {
	void *buf;
	unsigned long size;
	efi_guid_t vendor;
	s16 *name;
};

static int efivarfs_open(struct device_d *dev, FILE *f, const char *filename)
{
	struct efivars_file *efile;
	efi_status_t efiret;
	int ret;
	uint32_t attributes;

	efile = xzalloc(sizeof(*efile));

	ret = efivarfs_parse_filename(filename, &efile->vendor, &efile->name);
	if (ret)
		return -ENOENT;

	efiret = RT->get_variable(efile->name, &efile->vendor, &attributes, &efile->size, NULL);
	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL) {
		ret = -efi_errno(efiret);
		goto out;
	}

	efile->buf = malloc(efile->size + sizeof(uint32_t));
	if (!efile->buf) {
		ret = -ENOMEM;
		goto out;
	}

	efiret = RT->get_variable(efile->name, &efile->vendor, NULL, &efile->size,
			efile->buf + sizeof(uint32_t));
	if (EFI_ERROR(efiret)) {
		ret = -efi_errno(efiret);
		goto out;
	}

	*(uint32_t *)efile->buf = attributes;

	f->size = efile->size + sizeof(uint32_t);
	f->inode = efile;

	return 0;

out:
	free(efile->buf);
	free(efile);

	return ret;
}

static int efivarfs_close(struct device_d *dev, FILE *f)
{
	struct efivars_file *efile = f->inode;

	free(efile->buf);
	free(efile);

	return 0;
}

static int efivarfs_read(struct device_d *_dev, FILE *f, void *buf, size_t insize)
{
	struct efivars_file *efile = f->inode;

	memcpy(buf, efile->buf + f->pos, insize);

	return insize;
}

static loff_t efivarfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;

	return f->pos;
}

static DIR *efivarfs_opendir(struct device_d *dev, const char *pathname)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efivarfs_dir *edir;

	edir = xzalloc(sizeof(*edir));
	edir->current = priv->inodes.next;

	return &edir->dir;
}

static struct dirent *efivarfs_readdir(struct device_d *dev, DIR *dir)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efivarfs_dir *edir = container_of(dir, struct efivarfs_dir, dir);
	struct efivarfs_inode *inode;

	if (edir->current == &priv->inodes)
		return NULL;

	inode = list_entry(edir->current, struct efivarfs_inode, node);

	strcpy(dir->d.d_name, inode->name);

	edir->current = edir->current->next;

	return &dir->d;
}

static int efivarfs_closedir(struct device_d *dev, DIR *dir)
{
	struct efivarfs_dir *edir = container_of(dir, struct efivarfs_dir, dir);

	free(edir);

	return 0;
}

static int efivarfs_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	efi_guid_t vendor;
	s16 *name;
	efi_status_t efiret;
	unsigned long size = 0;
	int ret;

	ret = efivarfs_parse_filename(filename, &vendor, &name);
	if (ret)
		return -ENOENT;

	efiret = RT->get_variable(name, &vendor, NULL, &size, NULL);

	free(name);

	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL)
		return -efi_errno(efiret);

	s->st_mode = 00666 | S_IFREG;
	s->st_size = size;

	return 0;
}

static int efivarfs_probe(struct device_d *dev)
{
	efi_status_t efiret;
	efi_guid_t vendor;
	s16 name[1024];
	unsigned long size;
	unsigned char *name8;
	struct efivarfs_priv *priv;

	name[0] = 0;

	priv = xzalloc(sizeof(*priv));
	INIT_LIST_HEAD(&priv->inodes);

	while (1) {
		struct efivarfs_inode *inode;

		size = sizeof(name);
		efiret = RT->get_next_variable(&size, name, &vendor);
		if (EFI_ERROR(efiret))
			break;

		inode = xzalloc(sizeof(*inode));
		name8 = strdup_wchar_to_char(name);

		inode->name = asprintf("%s-%pUl", name8, &vendor);
		free(name8);

		list_add_tail(&inode->node, &priv->inodes);
	}

	dev->priv = priv;

	return 0;
}

static void efivarfs_remove(struct device_d *dev)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efivarfs_inode *inode, *tmp;

	list_for_each_entry_safe(inode, tmp, &priv->inodes, node) {
		free(inode->name);
		free(inode);
	}

	free(priv);
}

static struct fs_driver_d efivarfs_driver = {
	.open      = efivarfs_open,
	.close     = efivarfs_close,
	.read      = efivarfs_read,
	.lseek     = efivarfs_lseek,
	.opendir   = efivarfs_opendir,
	.readdir   = efivarfs_readdir,
	.closedir  = efivarfs_closedir,
	.stat      = efivarfs_stat,
	.drv = {
		.probe  = efivarfs_probe,
		.remove = efivarfs_remove,
		.name = "efivarfs",
	}
};

static int efivarfs_init(void)
{
	return register_fs_driver(&efivarfs_driver);
}

coredevice_initcall(efivarfs_init);
