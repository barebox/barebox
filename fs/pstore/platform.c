// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/fs/pstore/platform.c?id=bf4afc53b77aeaa48b5409da5c8da6bb4eff7f43
/*
 * Persistent Storage - platform driver interface parts.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (C) 2010 Intel Corporation <tony.luck@intel.com>
 */

#define pr_fmt(fmt) "pstore: " fmt

#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <init.h>
#include <linux/kmsg_dump.h>
#include <linux/module.h>
#include <linux/pstore.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/array_size.h>
#include <linux/printk.h>
#include <console.h>

#include "internal.h"

/* Names should be in the same order as the enum pstore_type_id */
static const char * const pstore_type_names[] = {
	"dmesg",
	"mce",
	"console",
	"ftrace",
	"rtas",
	"powerpc-ofw",
	"powerpc-common",
	"pmsg",
	"powerpc-opal",
};

/*
 * psinfo_lock protects "psinfo" during calls to
 * pstore_register(), pstore_unregister(), and
 * the filesystem mount/unmount routines.
 */
static DEFINE_MUTEX(psinfo_lock);
struct pstore_info *psinfo;

static char *backend;

static bool pstore_ready;

bool pstore_is_ready(void)
{
	return pstore_ready;
}
EXPORT_SYMBOL(pstore_is_ready);

const char *pstore_type_to_name(enum pstore_type_id type)
{
	BUILD_BUG_ON(ARRAY_SIZE(pstore_type_names) != PSTORE_TYPE_MAX);

	if (WARN_ON_ONCE(type >= PSTORE_TYPE_MAX))
		return "unknown";

	return pstore_type_names[type];
}
EXPORT_SYMBOL_GPL(pstore_type_to_name);

enum pstore_type_id pstore_name_to_type(const char *name)
{
	int i;

	for (i = 0; i < PSTORE_TYPE_MAX; i++) {
		if (!strcmp(pstore_type_names[i], name))
			return i;
	}

	return PSTORE_TYPE_MAX;
}
EXPORT_SYMBOL_GPL(pstore_name_to_type);

void pstore_record_init(struct pstore_record *record,
			struct pstore_info *psinfo)
{
	memset(record, 0, sizeof(*record));

	record->psi = psinfo;
}

#ifdef CONFIG_FS_PSTORE_CONSOLE
static void pstore_console_write(const char *s, unsigned c)
{
	struct pstore_record record;

	if (!c)
		return;

	pstore_record_init(&record, psinfo);
	record.type = PSTORE_TYPE_CONSOLE;

	record.buf = (char *)s;
	record.size = c;
	psinfo->write(&record);
}

void pstore_log(const char *str)
{
	static int in_pstore;

	if (!pstore_ready)
		return;

	if (in_pstore)
		return;

	in_pstore = 1;

	pstore_console_write(str, strlen(str));

	in_pstore = 0;
}

static void pstore_register_console(void)
{
	struct log_entry *log, *tmp;

	if (IS_ENABLED(CONFIG_CONSOLE_NONE))
		return;

	list_for_each_entry_safe(log, tmp, &barebox_logbuf, list)
		pstore_console_write(log->msg, strlen(log->msg));
}
#else
static void pstore_register_console(void) {}
#endif

/*
 * platform specific persistent storage driver registers with
 * us here. If pstore is already mounted, call the platform
 * read function right away to populate the file system. If not
 * then the pstore mount code will call us later to fill out
 * the file system.
 */
int pstore_register(struct pstore_info *psi)
{
	char *new_backend;

	if (backend && strcmp(backend, psi->name)) {
		pr_warn("backend '%s' already in use: ignoring '%s'\n",
			backend, psi->name);
		return -EBUSY;
	}

	/* Sanity check flags. */
	if (!psi->flags) {
		pr_warn("backend '%s' must support at least one frontend\n",
			psi->name);
		return -EINVAL;
	}

	/* Check for required functions. */
	if (!psi->read || !psi->write) {
		pr_warn("backend '%s' must implement read() and write()\n",
			psi->name);
		return -EINVAL;
	}

	new_backend = kstrdup(psi->name, GFP_KERNEL);
	if (!new_backend)
		return -ENOMEM;

	mutex_lock(&psinfo_lock);
	if (psinfo) {
		pr_warn("backend '%s' already loaded: ignoring '%s'\n",
			psinfo->name, psi->name);
		mutex_unlock(&psinfo_lock);
		kfree(new_backend);
		return -EBUSY;
	}

	psinfo = psi;
	mutex_init(&psinfo->read_mutex);
	raw_spin_lock_init(&psinfo->buf_lock);

	pstore_get_records(0);

	if (psi->flags & PSTORE_FLAGS_CONSOLE)
		pstore_register_console();
	if (psi->flags & PSTORE_FLAGS_FTRACE)
		pstore_register_ftrace();
	if (psi->flags & PSTORE_FLAGS_PMSG)
		pstore_register_pmsg();

	/*
	 * Update the module parameter backend, so it is visible
	 * through /sys/module/pstore/parameters/backend
	 */
	backend = new_backend;
	pstore_ready = true;

	pr_info("Registered %s as persistent store backend\n", psi->name);

	mutex_unlock(&psinfo_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(pstore_register);

/*
 * Read all the records from one persistent store backend. Create
 * files in our filesystem.  Don't warn about -EEXIST errors
 * when we are re-scanning the backing store looking to add new
 * error records.
 */
void pstore_get_backend_records(struct pstore_info *psi,
				struct dentry *root, int quiet)
{
	int failed = 0;
	unsigned int stop_loop = 65536;

	if (!psi)
		return;

	mutex_lock(&psi->read_mutex);
	if (psi->open && psi->open(psi))
		goto out;

	/*
	 * Backend callback read() allocates record.buf. decompress_record()
	 * may reallocate record.buf. On success, pstore_mkfile() will keep
	 * the record.buf, so free it only on failure.
	 */
	for (; stop_loop; stop_loop--) {
		struct pstore_record *record;
		int rc;

		record = kzalloc_obj(*record);
		if (!record) {
			pr_err("out of memory creating record\n");
			break;
		}
		pstore_record_init(record, psi);

		record->size = psi->read(record);

		/* No more records left in backend? */
		if (record->size <= 0) {
			kfree(record);
			break;
		}

		rc = pstore_mkfile(root, record);
		if (rc) {
			/* pstore_mkfile() did not take record, so free it. */
			kvfree(record->buf);
			kfree(record->priv);
			kfree(record);
			if (rc != -EEXIST || !quiet)
				failed++;
		}
	}
	if (psi->close)
		psi->close(psi);
out:
	mutex_unlock(&psi->read_mutex);

	if (failed)
		pr_warn("failed to create %d record(s) from '%s'\n",
			failed, psi->name);
	if (!stop_loop)
		pr_err("looping? Too many records seen from '%s'\n",
			psi->name);
}

MODULE_AUTHOR("Tony Luck <tony.luck@intel.com>");
MODULE_DESCRIPTION("Persistent Storage - platform driver interface");
MODULE_LICENSE("GPL");
