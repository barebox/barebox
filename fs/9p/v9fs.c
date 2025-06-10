// SPDX-License-Identifier: GPL-2.0-only
/*
 *  This file contains functions assisting in mapping VFS to 9P2000
 *
 *  Copyright (C) 2004-2008 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/uidgid.h>
#include <linux/kstrtox.h>
#include <linux/parser.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>
#include <net/9p/transport.h>
#include "v9fs.h"
#include "v9fs_vfs.h"

static DEFINE_SPINLOCK(v9fs_sessionlist_lock);
static LIST_HEAD(v9fs_sessionlist);

/*
 * Option Parsing (code inspired by NFS code)
 *  NOTE: each transport will parse its own options
 */

enum {
	/* Options that take integer arguments */
	Opt_debug, Opt_dfltuid, Opt_dfltgid, Opt_afid,
	/* String options */
	Opt_uname, Opt_remotename,
	/* Access options */
	Opt_access,
	/* Error token */
	Opt_err
};

static const match_table_t tokens = {
	{Opt_debug, "debug=%x"},
	{Opt_dfltuid, "dfltuid=%u"},
	{Opt_dfltgid, "dfltgid=%u"},
	{Opt_afid, "afid=%u"},
	{Opt_uname, "uname=%s"},
	{Opt_remotename, "aname=%s"},
	{Opt_access, "access=%s"},
	{Opt_err, NULL}
};

/*
 * Display the mount options in /proc/mounts.
 */
int v9fs_show_options(struct dentry *root)
{
	struct v9fs_session_info *v9ses = root->d_sb->s_fs_info;

	/* TODO: generate Linux bootarg */

	if (v9ses->debug)
		printf(",debug=%x", v9ses->debug);
	printf(",dfltuid=%u",
		   from_kuid(v9ses->dfltuid));
	printf(",dfltgid=%u",
		   from_kgid(v9ses->dfltgid));
	if (v9ses->afid != ~0)
		printf(",afid=%u", v9ses->afid);
	if (strcmp(v9ses->uname, V9FS_DEFUSER) != 0)
		printf(",uname=%s", v9ses->uname);
	if (strcmp(v9ses->aname, V9FS_DEFANAME) != 0)
		printf(",aname=%s", v9ses->aname);

	switch (v9ses->flags & V9FS_ACCESS_MASK) {
	case V9FS_ACCESS_USER:
		printf(",access=user");
		break;
	case V9FS_ACCESS_ANY:
		printf(",access=any");
		break;
	case V9FS_ACCESS_CLIENT:
		printf(",access=client");
		break;
	case V9FS_ACCESS_SINGLE:
		printf(",access=%u",
			   from_kuid(v9ses->uid));
		break;
	}

	return p9_show_client_options(v9ses->clnt);
}

/**
 * v9fs_parse_options - parse mount options into session structure
 * @v9ses: existing v9fs session information
 * @opts: The mount option string
 *
 * Return 0 upon success, -ERRNO upon failure.
 */

static int v9fs_parse_options(struct v9fs_session_info *v9ses, char *opts)
{
	char *options, *tmp_options;
	substring_t args[MAX_OPT_ARGS];
	char *p;
	int option = 0;
	char *s;
	int ret = 0;

	/* setup defaults */
	v9ses->afid = ~0;
	v9ses->debug = 0;

	if (!opts)
		return 0;

	tmp_options = kstrdup(opts, GFP_KERNEL);
	if (!tmp_options) {
		ret = -ENOMEM;
		goto fail_option_alloc;
	}
	options = tmp_options;

	while ((p = strsep(&options, ",")) != NULL) {
		int token, r;

		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_debug:
			r = match_int(&args[0], &option);
			if (r < 0) {
				p9_debug(P9_DEBUG_ERROR,
					 "integer field, but no integer?\n");
				ret = r;
			} else {
				v9ses->debug = option;
#ifdef CONFIG_NET_9P_DEBUG
				p9_debug_level = option;
#endif
			}
			break;

		case Opt_dfltuid:
			r = match_int(&args[0], &option);
			if (r < 0) {
				p9_debug(P9_DEBUG_ERROR,
					 "integer field, but no integer?\n");
				ret = r;
				continue;
			}
			v9ses->dfltuid = make_kuid(option);
			break;
		case Opt_dfltgid:
			r = match_int(&args[0], &option);
			if (r < 0) {
				p9_debug(P9_DEBUG_ERROR,
					 "integer field, but no integer?\n");
				ret = r;
				continue;
			}
			v9ses->dfltgid = make_kgid(option);
			break;
		case Opt_afid:
			r = match_int(&args[0], &option);
			if (r < 0) {
				p9_debug(P9_DEBUG_ERROR,
					 "integer field, but no integer?\n");
				ret = r;
			} else {
				v9ses->afid = option;
			}
			break;
		case Opt_uname:
			kfree(v9ses->uname);
			v9ses->uname = match_strdup(&args[0]);
			if (!v9ses->uname) {
				ret = -ENOMEM;
				goto free_and_return;
			}
			break;
		case Opt_remotename:
			kfree(v9ses->aname);
			v9ses->aname = match_strdup(&args[0]);
			if (!v9ses->aname) {
				ret = -ENOMEM;
				goto free_and_return;
			}
			break;
		case Opt_access:
			s = match_strdup(&args[0]);
			if (!s) {
				ret = -ENOMEM;
				p9_debug(P9_DEBUG_ERROR,
					 "problem allocating copy of access arg\n");
				goto free_and_return;
			}

			v9ses->flags &= ~V9FS_ACCESS_MASK;
			if (strcmp(s, "user") == 0)
				v9ses->flags |= V9FS_ACCESS_USER;
			else if (strcmp(s, "any") == 0)
				v9ses->flags |= V9FS_ACCESS_ANY;
			else if (strcmp(s, "client") == 0) {
				v9ses->flags |= V9FS_ACCESS_CLIENT;
			} else {
				uid_t uid;

				v9ses->flags |= V9FS_ACCESS_SINGLE;
				r = kstrtouint(s, 10, &uid);
				if (r) {
					ret = r;
					pr_info("Unknown access argument %s: %d\n",
						s, r);
					kfree(s);
					continue;
				}
				v9ses->uid = make_kuid(uid);
			}

			kfree(s);
			break;

		default:
			continue;
		}
	}

free_and_return:
	kfree(tmp_options);
fail_option_alloc:
	return ret;
}

/**
 * v9fs_session_init - initialize session
 * @v9ses: session information structure
 * @dev_name: device being mounted
 * @data: options
 *
 */

struct p9_fid *v9fs_session_init(struct v9fs_session_info *v9ses,
		  const char *dev_name, char *data)
{
	struct p9_fid *fid;
	int rc = -ENOMEM;

	v9ses->uname = kstrdup(V9FS_DEFUSER, GFP_KERNEL);
	if (!v9ses->uname)
		goto err_names;

	v9ses->aname = kstrdup(V9FS_DEFANAME, GFP_KERNEL);
	if (!v9ses->aname)
		goto err_names;
	init_rwsem(&v9ses->rename_sem);

	v9ses->uid = INVALID_UID;
	v9ses->dfltuid = V9FS_DEFUID;
	v9ses->dfltgid = V9FS_DEFGID;

	v9ses->clnt = p9_client_create(dev_name, data);
	if (IS_ERR(v9ses->clnt)) {
		rc = PTR_ERR(v9ses->clnt);
		p9_debug(P9_DEBUG_ERROR, "problem initializing 9p client\n");
		goto err_names;
	}

	v9ses->flags = V9FS_ACCESS_USER;

	v9ses->flags = V9FS_ACCESS_CLIENT;
	v9ses->flags |= V9FS_PROTO_2000L;

	rc = v9fs_parse_options(v9ses, data);
	if (rc < 0)
		goto err_clnt;

	v9ses->maxdata = v9ses->clnt->msize - P9_IOHDRSZ;

	fid = p9_client_attach(v9ses->clnt, NULL, v9ses->uname, INVALID_UID,
							v9ses->aname);
	if (IS_ERR(fid)) {
		rc = PTR_ERR(fid);
		p9_debug(P9_DEBUG_ERROR, "cannot attach\n");
		goto err_clnt;
	}

	if ((v9ses->flags & V9FS_ACCESS_MASK) == V9FS_ACCESS_SINGLE)
		fid->uid = v9ses->uid;
	else
		fid->uid = INVALID_UID;

	spin_lock(&v9fs_sessionlist_lock);
	list_add(&v9ses->slist, &v9fs_sessionlist);
	spin_unlock(&v9fs_sessionlist_lock);

	return fid;

err_clnt:
	p9_client_destroy(v9ses->clnt);
err_names:
	kfree(v9ses->uname);
	kfree(v9ses->aname);
	return ERR_PTR(rc);
}

/**
 * v9fs_session_close - shutdown a session
 * @v9ses: session information structure
 *
 */

void v9fs_session_close(struct v9fs_session_info *v9ses)
{
	if (v9ses->clnt) {
		p9_client_destroy(v9ses->clnt);
		v9ses->clnt = NULL;
	}

	kfree(v9ses->uname);
	kfree(v9ses->aname);

	spin_lock(&v9fs_sessionlist_lock);
	list_del(&v9ses->slist);
	spin_unlock(&v9fs_sessionlist_lock);
}

/**
 * v9fs_session_cancel - terminate a session
 * @v9ses: session to terminate
 *
 * mark transport as disconnected and cancel all pending requests.
 */

void v9fs_session_cancel(struct v9fs_session_info *v9ses)
{
	p9_debug(P9_DEBUG_ERROR, "cancel session %p\n", v9ses);
	p9_client_disconnect(v9ses->clnt);
}

/**
 * v9fs_session_begin_cancel - Begin terminate of a session
 * @v9ses: session to terminate
 *
 * After this call we don't allow any request other than clunk.
 */

void v9fs_session_begin_cancel(struct v9fs_session_info *v9ses)
{
	p9_debug(P9_DEBUG_ERROR, "begin cancel session %p\n", v9ses);
	p9_client_begin_disconnect(v9ses->clnt);
}

static int init_v9fs(void)
{
	pr_info("Installing v9fs 9p2000 file system support\n");
	/* TODO: Setup list of registered transport modules */

	return register_fs_driver(&v9fs_driver);
}
coredevice_initcall(init_v9fs);

MODULE_AUTHOR("Latchesar Ionkov <lucho@ionkov.net>");
MODULE_AUTHOR("Eric Van Hensbergen <ericvh@gmail.com>");
MODULE_AUTHOR("Ron Minnich <rminnich@lanl.gov>");
MODULE_DESCRIPTION("9P Client File System");
MODULE_LICENSE("GPL");
