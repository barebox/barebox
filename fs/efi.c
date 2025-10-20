/*
 * efi.c - EFI filesystem mirror driver
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
#include <command.h>
#include <errno.h>
#include <linux/stat.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <wchar.h>
#include <efi.h>
#include <libfile.h>
#include <efi/efi-payload.h>
#include <efi/efi-device.h>
#include <linux/stddef.h>

struct efifs_priv {
	struct efi_file_handle *root_dir;
	struct efi_file_io_interface *protocol;
};

struct efifs_file {
	struct efi_file_handle *entry;
};

struct efifs_dir {
	DIR dir;
	struct efi_file_handle *entries;
};

static wchar_t *path_to_efi(const char *path)
{
	wchar_t *dst;
	wchar_t *ret;

	if (!*path)
		return xstrdup_char_to_wchar("\\");

	dst = xstrdup_char_to_wchar(path);
	if (!dst)
		return NULL;

	ret = dst;

	while (*dst) {
		if (*dst == '/')
			*dst = '\\';
		dst++;
	}

	return ret;
}

static int efifs_create(struct device *dev, const char *pathname, mode_t mode)
{
	struct efifs_priv *priv = dev->priv;
	wchar_t *efi_path = path_to_efi(pathname);
	struct efi_file_handle *entry;
	efi_status_t efiret;

	efiret = priv->root_dir->open(priv->root_dir, &entry, efi_path,
			EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
			0ULL);

	free(efi_path);

	if (EFI_ERROR(efiret)) {
		printf("%s %s: %s\n", __func__, pathname, efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	entry->close(entry);

	return 0;
}

static int efifs_unlink(struct device *dev, const char *pathname)
{
	struct efifs_priv *priv = dev->priv;
	wchar_t *efi_path = path_to_efi(pathname);
	struct efi_file_handle *entry;
	efi_status_t efiret;

	efiret = priv->root_dir->open(priv->root_dir, &entry, efi_path,
			EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0ULL);

	free(efi_path);

	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	efiret = entry->delete(entry);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static int efifs_mkdir(struct device *dev, const char *pathname)
{
	struct efifs_priv *priv = dev->priv;
	wchar_t *efi_path = path_to_efi(pathname);
	struct efi_file_handle *entry;
	efi_status_t efiret;

	efiret = priv->root_dir->open(priv->root_dir, &entry, efi_path,
			EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
			EFI_FILE_DIRECTORY);

	free(efi_path);

	if (EFI_ERROR(efiret)) {
		printf("%s %s: %s\n", __func__, pathname, efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	entry->close(entry);

	return 0;
}

static int efifs_rmdir(struct device *dev, const char *pathname)
{
	return efifs_unlink(dev, pathname);
}

static int efifs_open(struct device *dev, struct file *f, const char *filename)
{
	struct efifs_priv *priv = dev->priv;
	efi_status_t efiret;
	struct efifs_file *ufile;
	wchar_t *efi_path = path_to_efi(filename);
	struct efi_file_info *info;
	size_t bufsize = 1024;
	uint64_t efimode = EFI_FILE_MODE_READ;
	int ret;

	ufile = xzalloc(sizeof(*ufile));

	if (f->f_flags & O_ACCMODE)
		efimode |= EFI_FILE_MODE_WRITE;

	efiret = priv->root_dir->open(priv->root_dir, &ufile->entry, efi_path,
			efimode, 0ULL);
	if (EFI_ERROR(efiret)) {
		pr_err("%s: unable to Open %s: %s\n", __func__,
				filename, efi_strerror(efiret));
		free(ufile);
		return -efi_errno(efiret);
	}

	free(efi_path);

	info = xzalloc(1024);
	efiret = ufile->entry->get_info(ufile->entry, &efi_file_info_id, &bufsize, info);
	if (EFI_ERROR(efiret)) {
		pr_err("%s: unable to GetInfo %s: %s\n", __func__,
				filename, efi_strerror(efiret));
		ret = -efi_errno(efiret);
		goto out;
	}

	f->f_size = info->FileSize;

	free(info);
	f->private_data = ufile;

	return 0;
out:
	free(info);
	free(ufile);
	return ret;
}

static int efifs_close(struct device *dev, struct file *f)
{
	struct efifs_file *ufile = f->private_data;

	ufile->entry->close(ufile->entry);

	free(ufile);

	return 0;
}

static int efifs_read(struct file *f, void *buf, size_t insize)
{
	struct efifs_file *ufile = f->private_data;
	efi_status_t efiret;
	size_t bufsize = insize;

	efiret = ufile->entry->read(ufile->entry, &bufsize, buf);
	if (EFI_ERROR(efiret)) {
		return -efi_errno(efiret);
	}

	return bufsize;
}

static int efifs_write(struct file *f, const void *buf, size_t insize)
{
	struct efifs_file *ufile = f->private_data;
	efi_status_t efiret;
	size_t bufsize = insize;

	efiret = ufile->entry->write(ufile->entry, &bufsize, (void *)buf);
	if (EFI_ERROR(efiret)) {
		pr_err("%s: unable to write: %s\n", __func__, efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	return bufsize;
}

static int efifs_lseek(struct file *f, loff_t pos)
{
	struct efifs_file *ufile = f->private_data;
	efi_status_t efiret;

	efiret = ufile->entry->set_position(ufile->entry, pos);
	if (EFI_ERROR(efiret)) {
		return -efi_errno(efiret);
	}

	return 0;
}

static int efifs_truncate(struct file *f, loff_t size)
{
	struct efifs_file *ufile = f->private_data;
	efi_status_t efiret;
	struct efi_file_info *info;
	size_t bufsize = 1024;
	int ret;

	info = xzalloc(1024);

	efiret = ufile->entry->get_info(ufile->entry, &efi_file_info_id, &bufsize, info);
	if (EFI_ERROR(efiret)) {
		pr_err("%s: unable to GetInfo: %s\n", __func__, efi_strerror(efiret));
		ret = -efi_errno(efiret);
		goto out;
	}

	if (size > info->FileSize)
		return 0;

	info->FileSize = size;

	efiret = ufile->entry->set_info(ufile->entry, &efi_file_info_id, bufsize, info);
	if (EFI_ERROR(efiret)) {
		pr_err("%s: unable to SetInfo: %s\n", __func__, efi_strerror(efiret));
		ret = -efi_errno(efiret);
		goto out;
	}

	return 0;
out:
	return ret;
}

static DIR *efifs_opendir(struct device *dev, const char *pathname)
{
	struct efifs_priv *priv = dev->priv;
	efi_status_t efiret;
	struct efifs_dir *udir;
	wchar_t *efi_path = path_to_efi(pathname);

	udir = xzalloc(sizeof(*udir));

	efiret = priv->root_dir->open(priv->root_dir, &udir->entries, efi_path, EFI_FILE_MODE_READ, 0ULL);
	if (EFI_ERROR(efiret)) {
		free(udir);
		return NULL;
	}

	free(efi_path);

	return &udir->dir;
}

static struct dirent *efifs_readdir(struct device *dev, DIR *dir)
{
	struct efifs_dir *udir = container_of(dir, struct efifs_dir, dir);
	efi_status_t efiret;
	size_t bufsize = 256;
	s16 buf[256];
	struct efi_file_info *f;

	efiret = udir->entries->read(udir->entries, &bufsize, buf);
	if (EFI_ERROR(efiret) || bufsize == 0)
		return NULL;

	f = (struct efi_file_info *)buf;

	strcpy_wchar_to_char(dir->d.d_name, f->FileName);

	return &dir->d;
}

static int efifs_closedir(struct device *dev, DIR *dir)
{
	struct efifs_dir *udir = container_of(dir, struct efifs_dir, dir);

	udir->entries->close(udir->entries);

	free(dir);

	return 0;
}

static int efifs_stat(struct device *dev, const char *filename,
		      struct stat *s)
{
	struct efifs_priv *priv = dev->priv;
	wchar_t *efi_path;
	efi_status_t efiret;
	struct efi_file_handle *entry;
	struct efi_file_info *info;
	size_t bufsize = 1024;
	int ret;

	info = xzalloc(1024);

	efi_path = path_to_efi(filename);

	efiret = priv->root_dir->open(priv->root_dir, &entry, efi_path, EFI_FILE_MODE_READ, 0ULL);
	if (EFI_ERROR(efiret)) {
		ret = -ENOENT;
		goto out_free;
	}

	efiret = entry->get_info(entry, &efi_file_info_id, &bufsize, info);
	if (EFI_ERROR(efiret)) {
		pr_err("%s: unable to GetInfo %s: %s\n", __func__, filename,
				efi_strerror(efiret));
		ret = -efi_errno(efiret);
		goto out;
	}

	s->st_size = info->FileSize;
	s->st_mode = 00555;

	if (!(info->Attribute & EFI_FILE_READ_ONLY))
		s->st_mode |= 00222;

	if (info->Attribute & EFI_FILE_DIRECTORY)
		s->st_mode |= S_IFDIR;
	else
		s->st_mode |= S_IFREG;

	ret = 0;
out:
	entry->close(entry);
out_free:
	free(efi_path);
	free(info);

	return ret;
}

static int efifs_symlink(struct device *dev, const char *pathname,
			 const char *newpath)
{
	return -EROFS;
}

static int efifs_readlink(struct device *dev, const char *pathname,
			  char *buf, size_t bufsiz)
{
	return -ENOENT;
}

static int efifs_probe(struct device *dev)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct efifs_priv *priv;
	efi_status_t efiret;
	struct efi_file_handle *file;
	struct device *efi = get_device_by_name(fsdev->backingstore);
	struct efi_device *udev = container_of(efi, struct efi_device, dev);

	priv = xzalloc(sizeof(struct efifs_priv));
	priv->protocol = udev->protocol;
	dev->priv = priv;
	dev->parent = &udev->dev;

	efiret = priv->protocol->open_volume(priv->protocol, &file);
	if (EFI_ERROR(efiret)) {
		dev_err(dev, "failed to open volume: %s\n", efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	priv->root_dir = file;

	return 0;
}

static void efifs_remove(struct device *dev)
{
	free(dev->priv);
}

static const struct fs_legacy_ops efifs_ops = {
	.open      = efifs_open,
	.close     = efifs_close,
	.create    = efifs_create,
	.unlink    = efifs_unlink,
	.mkdir     = efifs_mkdir,
	.rmdir     = efifs_rmdir,
	.opendir   = efifs_opendir,
	.readdir   = efifs_readdir,
	.closedir  = efifs_closedir,
	.stat      = efifs_stat,
	.symlink   = efifs_symlink,
	.readlink  = efifs_readlink,
	.truncate  = efifs_truncate,
	.read      = efifs_read,
	.write     = efifs_write,
	.lseek     = efifs_lseek,
};

static struct fs_driver efifs_driver = {
	.legacy_ops = &efifs_ops,
	.drv = {
		.probe  = efifs_probe,
		.remove = efifs_remove,
		.name = "efifs",
	}
};

static int efifs_init(void)
{
	return register_fs_driver(&efifs_driver);
}

coredevice_initcall(efifs_init);

static unsigned index;

static int efi_fs_probe(struct efi_device *efidev)
{
	char buf[sizeof("/efi4294967295")];
	const char *path;
	char *device;
	int ret;
	struct efi_file_io_interface *volume;

	if (efi_loaded_image)
		BS->handle_protocol(efi_loaded_image->device_handle,
				&efi_simple_file_system_protocol_guid, (void*)&volume);

	if (efi_loaded_image && efidev->protocol == volume) {
		path = "/boot";
	} else {
		snprintf(buf, sizeof(buf), "/efi%u", index);
		path = buf;
	}

	device = basprintf("%s", dev_name(&efidev->dev));

	ret = make_directory(path);
	if (ret)
		goto out;

	ret = mount(device, "efifs", path, NULL);
	if (ret)
		goto out;

	index++;

	dev_info(&efidev->dev, "mounted on %s\n", path);

	ret = 0;
out:
	free(device);

	return ret;
}

static struct efi_driver efi_fs_driver = {
        .driver = {
		.name  = "efi-fs",
	},
        .probe = efi_fs_probe,
	.guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
};
fs_efi_driver(efi_fs_driver);
