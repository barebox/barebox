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

#ifdef CONFIG_FS_PSTORE_CONSOLE
static void pstore_console_write(const char *s, unsigned c)
{
	const char *e = s + c;

	while (s < e) {
		struct pstore_record record = {
			.type = PSTORE_TYPE_CONSOLE,
			.psi = psinfo,
		};

		if (c > psinfo->bufsize)
			c = psinfo->bufsize;

		record.buf = (char *)s;
		record.size = c;
		psinfo->write_buf(PSTORE_TYPE_CONSOLE, 0, &record.id, 0,
				  record.buf, 0, record.size, psinfo);
		s += c;
		c = e - s;
	}
}

static int pstore_console_puts(struct console_device *cdev, const char *s)
{
	pstore_console_write(s, strlen(s));
	return strlen(s);
}

static void pstore_console_putc(struct console_device *cdev, char c)
{
	const char s[1] = { c };

	pstore_console_write(s, 1);
}

static void pstore_console_capture_log(void)
{
	struct log_entry *log;

	list_for_each_entry(log, &barebox_logbuf, list)
		pstore_console_write(log->msg, strlen(log->msg));
}

static struct console_device *pstore_cdev;

static void pstore_register_console(void)
{
	struct console_device *cdev;
	int ret;

	cdev = xzalloc(sizeof(struct console_device));
	pstore_cdev = cdev;

	cdev->puts = pstore_console_puts;
	cdev->putc = pstore_console_putc;
	cdev->devname = "pstore";
	cdev->devid = DEVICE_ID_SINGLE;

        ret = console_register(cdev);
        if (ret)
                pr_err("registering failed with %s\n", strerror(-ret));

	pstore_console_capture_log();

	console_set_active(pstore_cdev, CONSOLE_STDOUT);
}
#else
static void pstore_register_console(void) {}
#endif

static int pstore_write_compat(struct pstore_record *record)
{
	return record->psi->write_buf(record->type, record->reason,
				      &record->id, record->part,
				      psinfo->buf, record->compressed,
				      record->size, record->psi);
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

	pstore_register_console();

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
