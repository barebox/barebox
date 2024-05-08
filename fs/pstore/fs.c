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
#include <libbb.h>
#include <rtc.h>
#include <libfile.h>
#include "internal.h"

struct list_head allpstore = LIST_HEAD_INIT(allpstore);

struct pstore_private {
	char name[PSTORE_NAMELEN];
	struct list_head list;
	struct pstore_info *psi;
	enum pstore_type_id type;
	u64	id;
	int	count;
	ssize_t	size;
	ssize_t pos;
	char	data[];
};

/*
 * Make a regular file in the root directory of our file system.
 * Load it up with "size" bytes of data from "buf".
 * Set the mtime & ctime to the date that this record was originally stored.
 */
int pstore_mkfile(struct pstore_record *record)
{
	struct pstore_private	*private, *pos;
	size_t			size = record->size;

	list_for_each_entry(pos, &allpstore, list) {
		if (pos->type == record->type &&
		    pos->id == record->id &&
		    pos->psi == record->psi)
			return -EEXIST;
	}

	private = xzalloc(sizeof(*private) + size);
	private->type = record->type;
	private->id = record->id;
	private->count = record->count;
	private->psi = record->psi;

	switch (record->type) {
	case PSTORE_TYPE_DMESG:
		scnprintf(private->name, sizeof(private->name),
			  "dmesg-%s-%lld%s", record->psi->name, record->id,
			  record->compressed ? ".enc.z" : "");
		break;
	case PSTORE_TYPE_CONSOLE:
		scnprintf(private->name, sizeof(private->name),
			  "console-%s-%lld", record->psi->name, record->id);
		break;
	case PSTORE_TYPE_FTRACE:
		scnprintf(private->name, sizeof(private->name),
			  "ftrace-%s-%lld", record->psi->name, record->id);
		break;
	case PSTORE_TYPE_MCE:
		scnprintf(private->name, sizeof(private->name),
			  "mce-%s-%lld", record->psi->name, record->id);
		break;
	case PSTORE_TYPE_PPC_RTAS:
		scnprintf(private->name, sizeof(private->name),
			  "rtas-%s-%lld", record->psi->name, record->id);
		break;
	case PSTORE_TYPE_PPC_OF:
		scnprintf(private->name, sizeof(private->name),
			  "powerpc-ofw-%s-%lld", record->psi->name, record->id);
		break;
	case PSTORE_TYPE_PPC_COMMON:
		scnprintf(private->name, sizeof(private->name),
			  "powerpc-common-%s-%lld", record->psi->name,
			  record->id);
		break;
	case PSTORE_TYPE_PMSG:
		scnprintf(private->name, sizeof(private->name),
			  "pmsg-%s-%lld", record->psi->name, record->id);
		break;
	case PSTORE_TYPE_UNKNOWN:
		scnprintf(private->name, sizeof(private->name),
			  "unknown-%s-%lld", record->psi->name, record->id);
		break;
	default:
		scnprintf(private->name, sizeof(private->name),
			  "type%d-%s-%lld", record->type, record->psi->name,
			  record->id);
		break;
	}

	memcpy(private->data, record->buf, size);
	private->size = size;

	list_add(&private->list, &allpstore);

	return 0;
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

static int pstore_open(struct device *dev, FILE *file, const char *filename)
{
	struct list_head *head = dev->priv;
	struct pstore_private *d;

	d = pstore_get_by_name(head, filename);
	if (!d)
		return -ENOENT;

	file->size = d->size;
	file->priv = d;
	d->pos = 0;

	return 0;
}

static int pstore_close(struct device *dev, FILE *file)
{
	return 0;
}

static int pstore_read(struct device *dev, FILE *file, void *buf,
		       size_t insize)
{
	struct pstore_private *d = file->priv;

	memcpy(buf, &d->data[d->pos], insize);
	d->pos += insize;

	return insize;
}

static int pstore_lseek(struct device *dev, FILE *file, loff_t pos)
{
	struct pstore_private *d = file->priv;

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

	if (!d->psi->erase)
		return -EPERM;

	ret = d->psi->erase(d->type, d->id, d->count, d->psi);
	if (ret)
		return ret;

	list_del(&d->list);
	free(d);

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
		free(d);
	}
}

static int pstore_probe(struct device *dev)
{
	struct list_head *priv = &allpstore;

	dev->priv = priv;

	dev_dbg(dev, "mounted pstore\n");

	return 0;
}

static struct fs_driver pstore_driver = {
	.open      = pstore_open,
	.close     = pstore_close,
	.read      = pstore_read,
	.lseek     = pstore_lseek,
	.unlink    = pstore_unlink,
	.opendir   = pstore_opendir,
	.readdir   = pstore_readdir,
	.closedir  = pstore_closedir,
	.stat      = pstore_stat,
	.flags     = FS_DRIVER_NO_DEV,
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
