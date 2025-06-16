// SPDX-License-Identifier: GPL-2.0-only
/*
 *  9P entry point
 *
 *  Copyright (C) 2007 by Latchesar Ionkov <lucho@ionkov.net>
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <globalvar.h>
#include <magicvar.h>
#include <net/9p/9p.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/parser.h>
#include <net/9p/client.h>
#include <net/9p/transport.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#ifdef CONFIG_NET_9P_DEBUG
unsigned int p9_debug_level;	/* feature-rific global debug level  */
BAREBOX_MAGICVAR(plan9debug, "9P debugging level");

void _p9_debug(enum p9_debug_flags level, const char *func,
	       const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if ((p9_debug_level & level) != level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	if (level == P9_DEBUG_9P)
		pr_notice("%pV", &vaf);
	else
		pr_notice("-- %s %pV", func, &vaf);

	va_end(args);
}
EXPORT_SYMBOL(_p9_debug);

static inline void p9_add_debug_globalvar(void)
{
	globalvar_add_simple_int("9p.debug", &p9_debug_level, "%d");
}
#else
static inline void p9_add_debug_globalvar(void)
{
}
#endif

/* Dynamic Transport Registration Routines */

static DEFINE_SPINLOCK(v9fs_trans_lock);
static LIST_HEAD(v9fs_trans_list);

/**
 * v9fs_register_trans - register a new transport with 9p
 * @m: structure describing the transport module and entry points
 *
 */
void v9fs_register_trans(struct p9_trans_module *m)
{
	spin_lock(&v9fs_trans_lock);
	list_add_tail(&m->list, &v9fs_trans_list);
	spin_unlock(&v9fs_trans_lock);
}
EXPORT_SYMBOL(v9fs_register_trans);

/**
 * v9fs_unregister_trans - unregister a 9p transport
 * @m: the transport to remove
 *
 */
void v9fs_unregister_trans(struct p9_trans_module *m)
{
	spin_lock(&v9fs_trans_lock);
	list_del_init(&m->list);
	spin_unlock(&v9fs_trans_lock);
}
EXPORT_SYMBOL(v9fs_unregister_trans);

/**
 * v9fs_get_trans_by_name - get transport with the matching name
 * @s: string identifying transport
 *
 */
struct p9_trans_module *v9fs_get_trans_by_name(const char *s)
{
	struct p9_trans_module *t, *found = NULL;

	spin_lock(&v9fs_trans_lock);

	list_for_each_entry(t, &v9fs_trans_list, list)
		if (strcmp(t->name, s) == 0) {
			found = t;
			break;
		}

	spin_unlock(&v9fs_trans_lock);

	return found;
}
EXPORT_SYMBOL(v9fs_get_trans_by_name);

static const char * const v9fs_default_transports[] = {
	"virtio", "tcp", "fd", "unix", "xen", "rdma",
};

/**
 * v9fs_get_default_trans - get the default transport
 *
 */

struct p9_trans_module *v9fs_get_default_trans(void)
{
	struct p9_trans_module *t, *found = NULL;
	int i;

	spin_lock(&v9fs_trans_lock);

	list_for_each_entry(t, &v9fs_trans_list, list)
		if (t->def) {
			found = t;
			break;
		}

	if (!found)
		list_for_each_entry(t, &v9fs_trans_list, list) {
			found = t;
			break;
		}

	spin_unlock(&v9fs_trans_lock);

	for (i = 0; !found && i < ARRAY_SIZE(v9fs_default_transports); i++)
		found = v9fs_get_trans_by_name(v9fs_default_transports[i]);

	return found;
}
EXPORT_SYMBOL(v9fs_get_default_trans);

/**
 * init_p9 - Initialize module
 *
 */
static int __init init_p9(void)
{
	int ret;

	p9_add_debug_globalvar();

	ret = p9_client_init();
	if (ret)
		return ret;

	pr_info("Installing 9P2000 support\n");

	return ret;
}

fs_initcall(init_p9)

MODULE_AUTHOR("Latchesar Ionkov <lucho@ionkov.net>");
MODULE_AUTHOR("Eric Van Hensbergen <ericvh@gmail.com>");
MODULE_AUTHOR("Ron Minnich <rminnich@lanl.gov>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Plan 9 Resource Sharing Support (9P2000)");
