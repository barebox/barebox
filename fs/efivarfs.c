/*
 * efivars.c - EFI variable filesystem
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <wchar.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <efi/services.h>
#include <efi/mode.h>
#include <efi/variable.h>
#include <efi/error.h>
#include <efi/guid.h>

struct efivarfs_inode {
	s16 *name;
	efi_guid_t vendor;
	char *full_name; /* name including vendor namespacing */
	struct list_head node;
};

struct efivarfs_dir {
	struct list_head *current;
	DIR dir;
};

struct efivarfs_priv {
	struct list_head inodes;
	struct efi_runtime_services *rt;
};

static int efivars_create(struct device *dev, const char *pathname,
			  mode_t mode)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efi_runtime_services *rt = priv->rt;
	struct efivarfs_inode *inode;
	efi_guid_t vendor;
	efi_status_t efiret;
	u8 dummydata;
	char *name8;
	s16 *name;
	int ret;

	if (pathname[0] == '/')
		pathname++;

	/* deny creating files with other vendor GUID than our own */
	ret = efivarfs_parse_filename(pathname, &vendor, &name);
	if (ret)
		return -ENOENT;

	if (efi_guidcmp(vendor, EFI_BAREBOX_VENDOR_GUID))
		return -EPERM;

	inode = xzalloc(sizeof(*inode));
	inode->name = name;
	inode->vendor = vendor;


	name8 = xstrdup_wchar_to_char(inode->name);
	inode->full_name = basprintf("%s-%pUl", name8, &inode->vendor);
	free(name8);

	efiret = rt->set_variable(inode->name, &inode->vendor,
				  EFI_VARIABLE_NON_VOLATILE |
				  EFI_VARIABLE_BOOTSERVICE_ACCESS |
				  EFI_VARIABLE_RUNTIME_ACCESS,
				  1, &dummydata);
	if (EFI_ERROR(efiret)) {
		free(inode);
		return -efi_errno(efiret);
	}

	list_add_tail(&inode->node, &priv->inodes);

	return 0;
}

static int efivars_unlink(struct device *dev, const char *pathname)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efi_runtime_services *rt = priv->rt;
	struct efivarfs_inode *inode, *tmp;
	efi_status_t efiret;

	if (pathname[0] == '/')
		pathname++;

	list_for_each_entry_safe(inode, tmp, &priv->inodes, node) {
		if (!strcmp(inode->full_name, pathname)) {
			efiret = rt->set_variable(inode->name, &inode->vendor,
						  0, 0, NULL);
			if (EFI_ERROR(efiret))
				return -efi_errno(efiret);
			list_del(&inode->node);
			free(inode);
		}
	}

	return 0;
}

struct efivars_file {
	void *buf;
	size_t size;
	efi_guid_t vendor;
	s16 *name;
	u32 attributes;
	struct efivarfs_priv *priv;
};

static int efivarfs_open(struct device *dev, struct file *f, const char *filename)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efi_runtime_services *rt = priv->rt;
	struct efivars_file *efile;
	efi_status_t efiret;
	int ret;

	efile = xzalloc(sizeof(*efile));

	ret = efivarfs_parse_filename(filename, &efile->vendor, &efile->name);
	if (ret)
		return -ENOENT;

	efiret = rt->get_variable(efile->name, &efile->vendor,
				  NULL, &efile->size, NULL);
	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL) {
		ret = -efi_errno(efiret);
		goto out;
	}

	efile->buf = malloc(efile->size);
	if (!efile->buf) {
		ret = -ENOMEM;
		goto out;
	}

	efiret = rt->get_variable(efile->name, &efile->vendor,
				  &efile->attributes, &efile->size,
				  efile->buf);
	if (EFI_ERROR(efiret)) {
		ret = -efi_errno(efiret);
		goto out;
	}

	efile->priv = priv;

	f->f_size = efile->size;
	f->private_data = efile;

	return 0;

out:
	free(efile->buf);
	free(efile);

	return ret;
}

static int efivarfs_close(struct device *dev, struct file *f)
{
	struct efivars_file *efile = f->private_data;

	free(efile->buf);
	free(efile);

	return 0;
}

static int efivarfs_read(struct file *f, void *buf, size_t insize)
{
	struct efivars_file *efile = f->private_data;

	memcpy(buf, efile->buf + f->f_pos, insize);

	return insize;
}

static int efivarfs_write(struct file *f, const void *buf, size_t insize)
{
	struct efivars_file *efile = f->private_data;
	struct efi_runtime_services *rt = efile->priv->rt;
	efi_status_t efiret;

	if (efile->size < f->f_pos + insize) {
		efile->buf = realloc(efile->buf, f->f_pos + insize);
		efile->size = f->f_pos + insize;
	}

	memcpy(efile->buf + f->f_pos, buf, insize);

	efiret = rt->set_variable(efile->name, &efile->vendor,
				  efile->attributes,
				  efile->size ? efile->size : 1, efile->buf);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return insize;
}

static int efivarfs_truncate(struct file *f, loff_t size)
{
	struct efivars_file *efile = f->private_data;
	struct efi_runtime_services *rt = efile->priv->rt;
	efi_status_t efiret;

	efile->size = size;
	efile->buf = realloc(efile->buf, efile->size + sizeof(uint32_t));

	efiret = rt->set_variable(efile->name, &efile->vendor,
				  efile->attributes,
				  efile->size ? efile->size : 1, efile->buf);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static DIR *efivarfs_opendir(struct device *dev, const char *pathname)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efivarfs_dir *edir;

	edir = xzalloc(sizeof(*edir));
	edir->current = priv->inodes.next;

	return &edir->dir;
}

static struct dirent *efivarfs_readdir(struct device *dev, DIR *dir)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efivarfs_dir *edir = container_of(dir, struct efivarfs_dir, dir);
	struct efivarfs_inode *inode;

	if (edir->current == &priv->inodes)
		return NULL;

	inode = list_entry(edir->current, struct efivarfs_inode, node);

	strcpy(dir->d.d_name, inode->full_name);

	edir->current = edir->current->next;

	return &dir->d;
}

static int efivarfs_closedir(struct device *dev, DIR *dir)
{
	struct efivarfs_dir *edir = container_of(dir, struct efivarfs_dir, dir);

	free(edir);

	return 0;
}

static int efivarfs_stat(struct device *dev, const char *filename,
			 struct stat *s)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efi_runtime_services *rt = priv->rt;
	efi_guid_t vendor;
	s16 *name;
	efi_status_t efiret;
	size_t size = 0;
	int ret;

	ret = efivarfs_parse_filename(filename, &vendor, &name);
	if (ret)
		return -ENOENT;

	efiret = rt->get_variable(name, &vendor, NULL, &size, NULL);

	free(name);

	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL)
		return -efi_errno(efiret);

	s->st_mode = 00666 | S_IFREG;
	s->st_size = size;

	return 0;
}

static int efivarfs_probe(struct device *dev)
{
	efi_status_t efiret;
	efi_guid_t vendor;
	s16 name[1024];
	char *name8;
	size_t size;
	struct efivarfs_priv *priv;
	struct efi_runtime_services *rt;

	rt = efi_get_runtime_services();
	if (!rt)
		return -ENODEV;

	name[0] = 0;

	priv = xzalloc(sizeof(*priv));
	priv->rt = rt;
	INIT_LIST_HEAD(&priv->inodes);

	while (1) {
		struct efivarfs_inode *inode;

		size = sizeof(name);
		efiret = rt->get_next_variable(&size, name, &vendor);
		if (EFI_ERROR(efiret))
			break;

		inode = xzalloc(sizeof(*inode));
		inode->name = xstrdup_wchar(name);

		inode->vendor = vendor;

		name8 = xstrdup_wchar_to_char(inode->name);
		inode->full_name = basprintf("%s-%pUl", name8, &vendor);
		free(name8);

		list_add_tail(&inode->node, &priv->inodes);
	}

	dev->priv = priv;

	return 0;
}

static void efivarfs_remove(struct device *dev)
{
	struct efivarfs_priv *priv = dev->priv;
	struct efivarfs_inode *inode, *tmp;

	list_for_each_entry_safe(inode, tmp, &priv->inodes, node) {
		free(inode->name);
		free(inode);
	}

	free(priv);
}

static const struct fs_legacy_ops efivarfs_ops = {
	.open      = efivarfs_open,
	.close     = efivarfs_close,
	.create    = efivars_create,
	.unlink    = efivars_unlink,
	.opendir   = efivarfs_opendir,
	.readdir   = efivarfs_readdir,
	.closedir  = efivarfs_closedir,
	.stat      = efivarfs_stat,
	.read      = efivarfs_read,
	.write     = efivarfs_write,
	.truncate  = efivarfs_truncate,
};

static struct fs_driver efivarfs_driver = {
	.legacy_ops = &efivarfs_ops,
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
