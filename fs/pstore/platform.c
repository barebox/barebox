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
#include <console.h>
#include <malloc.h>
#include <linux/printk.h>
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

static int pstore_ready;

bool pstore_is_ready(void)
{
	return pstore_ready;
}
EXPORT_SYMBOL(pstore_is_ready);

void pstore_log(const char *str)
{
	uint64_t id;
	static int in_pstore;

	if (!IS_ENABLED(CONFIG_FS_PSTORE_CONSOLE))
		return;

	if (!pstore_ready)
		return;

	if (in_pstore)
		return;

	in_pstore = 1;

	psinfo->write_buf(PSTORE_TYPE_CONSOLE, 0, &id, 0,
			  str, 0, strlen(str), psinfo);

	in_pstore = 0;
}

static void pstore_console_capture_log(void)
{
	uint64_t id;
	struct log_entry *log, *tmp;

	if (IS_ENABLED(CONFIG_CONSOLE_NONE))
		return;

	list_for_each_entry_safe(log, tmp, &barebox_logbuf, list) {
		psinfo->write_buf(PSTORE_TYPE_CONSOLE, 0, &id, 0,
				  log->msg, 0, strlen(log->msg), psinfo);
	}
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

	psinfo = psi;
	mutex_init(&psinfo->read_mutex);
	spin_unlock(&pstore_lock);

	pstore_get_records(0);

	pstore_console_capture_log();
	pstore_ready = 1;

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
	struct pstore_record	record = { .psi = psi, };
	int			failed = 0, rc;
	int			unzipped_len = -1;

	if (!psi)
		return;

	mutex_lock(&psi->read_mutex);
	if (psi->open && psi->open(psi))
		goto out;

	while ((record.size = psi->read(&record)) > 0) {
		if (record.compressed &&
		    record.type == PSTORE_TYPE_DMESG) {
			pr_err("barebox does not have ramoops compression support\n");
			continue;
		}
		rc = pstore_mkfile(&record);
		if (unzipped_len < 0) {
			/* Free buffer other than big oops */
			kfree(record.buf);
			record.buf = NULL;
		} else
			unzipped_len = -1;
		if (rc && (rc != -EEXIST || !quiet))
			failed++;

		memset(&record, 0, sizeof(record));
		record.psi = psi;
	}
	if (psi->close)
		psi->close(psi);
out:
	mutex_unlock(&psi->read_mutex);

	if (failed)
		pr_warn("failed to load %d record(s) from '%s'\n",
			failed, psi->name);
}
