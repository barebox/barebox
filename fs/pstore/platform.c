/*
 * Persistent Storage - platform driver interface parts.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (C) 2010 Intel Corporation <tony.luck@intel.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt) "pstore: " fmt

#include <linux/types.h>
#include <linux/pstore.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <malloc.h>
#include <printk.h>
#include <module.h>

#include "internal.h"

struct pstore_info *psinfo;

static char *backend;

/* How much of the console log to snapshot */
static unsigned long kmsg_bytes = 10240;

void pstore_set_kmsg_bytes(int bytes)
{
	kmsg_bytes = bytes;
}

static int pstore_write_compat(enum pstore_type_id type,
			       enum kmsg_dump_reason reason,
			       u64 *id, unsigned int part, int count,
			       bool compressed, size_t size,
			       struct pstore_info *psi)
{
	return psi->write_buf(type, reason, id, part, psinfo->buf, compressed,
			     size, psi);
}

/*
 * platform specific persistent storage driver registers with
 * us here. If pstore is already mounted, call the platform
 * read function right away to populate the file system. If not
 * then the pstore mount code will call us later to fill out
 * the file system.
 *
 * Register with kmsg_dump to save last part of console log on panic.
 */
int pstore_register(struct pstore_info *psi)
{
	if (backend && strcmp(backend, psi->name))
		return -EPERM;

	spin_lock(&pstore_lock);
	if (psinfo) {
		spin_unlock(&pstore_lock);
		return -EBUSY;
	}

	if (!psi->write)
		psi->write = pstore_write_compat;
	psinfo = psi;
	mutex_init(&psinfo->read_mutex);
	spin_unlock(&pstore_lock);

	pstore_get_records(0);

	pr_info("Registered %s as persistent store backend\n", psi->name);

	return 0;
}
EXPORT_SYMBOL_GPL(pstore_register);

/*
 * Read all the records from the persistent store. Create
 * files in our filesystem.  Don't warn about -EEXIST errors
 * when we are re-scanning the backing store looking to add new
 * error records.
 */
void pstore_get_records(int quiet)
{
	struct pstore_info *psi = psinfo;
	char			*buf = NULL;
	ssize_t			size;
	u64			id;
	int			count;
	enum pstore_type_id	type;
	int			failed = 0, rc;
	bool			compressed;
	int			unzipped_len = -1;

	if (!psi)
		return;

	mutex_lock(&psi->read_mutex);
	if (psi->open && psi->open(psi))
		goto out;

	while ((size = psi->read(&id, &type, &count, &buf, &compressed,
				psi)) > 0) {
		if (compressed && (type == PSTORE_TYPE_DMESG)) {
			pr_err("barebox does not have ramoops compression support\n");
			continue;
		}
		rc = pstore_mkfile(type, psi->name, id, count, buf,
				  compressed, (size_t)size, psi);
		if (unzipped_len < 0) {
			/* Free buffer other than big oops */
			kfree(buf);
			buf = NULL;
		} else
			unzipped_len = -1;
		if (rc && (rc != -EEXIST || !quiet))
			failed++;
	}
	if (psi->close)
		psi->close(psi);
out:
	mutex_unlock(&psi->read_mutex);

	if (failed)
		pr_warn("failed to load %d record(s) from '%s'\n",
			failed, psi->name);
}
