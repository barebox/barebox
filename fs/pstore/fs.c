/*
 * Persistent Storage Barebox filesystem layer
 * Copyright © 2015 Pengutronix, Markus Pargmann <mpa@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <driver.h>
#include <fs.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/pstore.h>
#include <linux/slab.h>
#include <libbb.h>
#include <rtc.h>
#include <libfile.h>
#include "internal.h"

#define PSTORE_NAMELEN	64

struct list_head allpstore = LIST_HEAD_INIT(allpstore);

struct pstore_private {
	char name[PSTORE_NAMELEN];
	struct list_head list;
	struct pstore_record *record;
	ssize_t	size;
	ssize_t pos;
};

static void free_pstore_private(struct pstore_private *private)
{
	struct pstore_record *record = private->record;

	kvfree(record->buf);
	kfree(record->priv);
	kfree(record);
	free(private);
}

/*
 * Make a regular file in the root directory of our file system.
 * Load it up with "size" bytes of data from "buf".
 * On success, the file takes ownership of the record.
 */
int pstore_mkfile(struct dentry *root, struct pstore_record *record)
{
	struct pstore_private	*private, *pos;

	list_for_each_entry(pos, &allpstore, list) {
		if (pos->record->type == record->type &&
		    pos->record->id == record->id &&
		    pos->record->psi == record->psi)
			return -EEXIST;
	}

	private = xzalloc(sizeof(*private));
	private->record = record;
	private->size = record->size + record->ecc_notice_size;

	scnprintf(private->name, sizeof(private->name), "%s-%s-%llu%s",
		  pstore_type_to_name(record->type), record->psi->name,
		  record->id, record->compressed ? ".enc.z" : "");

	list_add(&private->list, &allpstore);

	return 0;
}

void pstore_get_records(int quiet)
{
	pstore_get_backend_records(psinfo, NULL, quiet);
}

static struct pstore_private *pstore_get_by_name(struct list_head *head,
						 const char *name)
{
	struct pstore_private *d;

	if (!name)
		return NULL;

	if (name[0] == '/')
		name++;

	list_for_each_entry(d, head, list) {
		if (strcmp(d->name, name) == 0)
			return d;
	}

	return NULL;
}

static int pstore_open(struct device *dev, struct file *file, const char *filename)
{
	struct list_head *head = dev->priv;
	struct pstore_private *d;

	d = pstore_get_by_name(head, filename);
	if (!d)
		return -ENOENT;

	file->f_size = d->size;
	file->private_data = d;
	d->pos = 0;

	return 0;
}

static int pstore_close(struct device *dev, struct file *file)
{
	return 0;
}

static int pstore_read(struct file *file, void *buf,
		       size_t insize)
{
	struct pstore_private *d = file->private_data;

	memcpy(buf, &d->record->buf[d->pos], insize);
	d->pos += insize;

	return insize;
}

static int pstore_lseek(struct file *file, loff_t pos)
{
	struct pstore_private *d = file->private_data;

	d->pos = pos;

	return 0;
}

static int pstore_unlink(struct device *dev, const char *filename)
{
	struct list_head *head = dev->priv;
	struct pstore_private *d;
	int ret;

	d = pstore_get_by_name(head, filename);
	if (!d)
		return -ENOENT;

	if (!d->record->psi->erase)
		return -EPERM;

	ret = d->record->psi->erase(d->record);
	if (ret)
		return ret;

	list_del(&d->list);
	free_pstore_private(d);

	return 0;
}

static DIR *pstore_opendir(struct device *dev, const char *pathname)
{
	DIR *dir;

	dir = xzalloc(sizeof(DIR));

	if (list_empty(&allpstore))
		return dir;

	dir->priv = list_first_entry(&allpstore, struct pstore_private, list);

	return dir;
}

static struct dirent *pstore_readdir(struct device *dev, DIR *dir)
{
	struct pstore_private *d = dir->priv;

	if (!d || &d->list == &allpstore)
		return NULL;

	strcpy(dir->d.d_name, d->name);
	dir->priv = list_entry(d->list.next, struct pstore_private, list);

	return &dir->d;
}

static int pstore_closedir(struct device *dev, DIR *dir)
{
	free(dir);

	return 0;
}

static int pstore_stat(struct device *dev, const char *filename,
		       struct stat *s)
{
	struct pstore_private *d;

	d = pstore_get_by_name(&allpstore, filename);
	if (!d)
		return -ENOENT;

	s->st_size = d->size;
	s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

	return 0;
}

static void pstore_remove(struct device *dev)
{
	struct pstore_private *d, *tmp;

	list_for_each_entry_safe(d, tmp, &allpstore, list) {
		free_pstore_private(d);
	}
}

static int pstore_probe(struct device *dev)
{
	struct list_head *priv = &allpstore;

	dev->priv = priv;

	dev_dbg(dev, "mounted pstore\n");

	return 0;
}

static const struct fs_legacy_ops pstore_ops = {
	.open      = pstore_open,
	.close     = pstore_close,
	.unlink    = pstore_unlink,
	.opendir   = pstore_opendir,
	.readdir   = pstore_readdir,
	.closedir  = pstore_closedir,
	.stat      = pstore_stat,
	.read      = pstore_read,
	.lseek     = pstore_lseek,
};

static struct fs_driver pstore_driver = {
	.legacy_ops = &pstore_ops,
	.type = filetype_uimage,
	.drv = {
		.probe  = pstore_probe,
		.remove = pstore_remove,
		.name = "pstore",
	}
};

static int pstore_init(void)
{
	return register_fs_driver(&pstore_driver);
}
coredevice_initcall(pstore_init);
