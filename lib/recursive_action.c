/* vi: set sw=8 ts=8: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */
#ifdef __BAREBOX__

#include <common.h>
#include <fs.h>
#include <linux/stat.h>
#include <malloc.h>
#include <libbb.h>
#include <xfuncs.h>

/*
 * Sorting not required for barebox itself currently, support for it is only added
 * for generating reproducible envfs images on the build host.
 */
#define DO_SORT(flags)	0

#else

#define DO_SORT(flags)	((flags) & ACTION_SORT)

#endif

#include <linux/list.h>
#include <linux/list_sort.h>

struct dirlist {
	char *dirname;
	struct list_head list;
};

/*
 * Walk down all the directories under the specified
 * location, and do something (something specified
 * by the fileAction and dirAction function pointers).
 *
 * Unfortunately, while nftw(3) could replace this and reduce
 * code size a bit, nftw() wasn't supported before GNU libc 2.1,
 * and so isn't sufficiently portable to take over since glibc2.1
 * is so stinking huge.
 */

static int true_action(const char *fileName, struct stat *statbuf,
						void* userData, int depth)
{
	return 1;
}

static int cmp_dirlist(void *priv, struct list_head *a, struct list_head *b)
{
	struct dirlist *ra, *rb;

	if (a == b)
		return 0;

	ra = list_entry(a, struct dirlist, list);
	rb = list_entry(b, struct dirlist, list);

	return strcmp(ra->dirname, rb->dirname);
}

/* fileAction return value of 0 on any file in directory will make
 * recursive_action() return 0, but it doesn't stop directory traversal
 * (fileAction/dirAction will be called on each file).
 *
 * if !depthFirst, dirAction return value of 0 (FALSE) or 2 (SKIP)
 * prevents recursion into that directory, instead
 * recursive_action() returns 0 (if FALSE) or 1 (if SKIP).
 *
 * followLinks=0/1 differs mainly in handling of links to dirs.
 * 0: lstat(statbuf). Calls fileAction on link name even if points to dir.
 * 1: stat(statbuf). Calls dirAction and optionally recurse on link to dir.
 */

int recursive_action(const char *fileName,
		unsigned flags,
		int (*fileAction)(const char *fileName, struct stat *statbuf, void* userData, int depth),
		int (*dirAction)(const char *fileName, struct stat *statbuf, void* userData, int depth),
		void* userData,
		const unsigned depth)
{
	struct stat statbuf;
	unsigned follow;
	int status;
	DIR *dir;
	struct dirent *next;
	struct dirlist *entry, *entry_tmp;
	LIST_HEAD(dirs);

	if (!fileAction) fileAction = true_action;
	if (!dirAction) dirAction = true_action;

	follow = ACTION_FOLLOWLINKS;
	if (depth == 0)
		follow = ACTION_FOLLOWLINKS;
	follow &= flags;
	status = (follow ? stat : lstat)(fileName, &statbuf);
	if (status < 0) {
#ifdef DEBUG_RECURS_ACTION
		bb_error_msg("status=%d followLinks=%d TRUE=%d",
				status, flags & ACTION_FOLLOWLINKS, TRUE);
#endif
		goto done_nak_warn;
	}

	/* If S_ISLNK(m), then we know that !S_ISDIR(m).
	 * Then we can skip checking first part: if it is true, then
	 * (!dir) is also true! */
	if ( /* (!(flags & ACTION_FOLLOWLINKS) && S_ISLNK(statbuf.st_mode)) || */
	 !S_ISDIR(statbuf.st_mode)
	) {
		return fileAction(fileName, &statbuf, userData, depth);
	}

	/* It's a directory (or a link to one, and followLinks is set) */

	if (!(flags & ACTION_RECURSE)) {
		return dirAction(fileName, &statbuf, userData, depth);
	}

	if (!(flags & ACTION_DEPTHFIRST)) {
		status = dirAction(fileName, &statbuf, userData, depth);
		if (!status) {
			goto done_nak_warn;
		}
		if (status == 2)
			return 1;
	}

	dir = opendir(fileName);
	if (!dir) {
		/* findutils-4.1.20 reports this */
		/* (i.e. it doesn't silently return with exit code 1) */
		/* To trigger: "find -exec rm -rf {} \;" */
		goto done_nak_warn;
	}

	status = 1;
	while ((next = readdir(dir)) != NULL) {
		char *nextFile = concat_subpath_file(fileName, next->d_name);
		if (nextFile == NULL)
			continue;

		if (DO_SORT(flags)) {
			struct dirlist *e = xmalloc(sizeof(*e));
			e->dirname = nextFile;
			list_add(&e->list, &dirs);
		} else {
			/* descend into it, forcing recursion. */
			if (!recursive_action(nextFile, flags | ACTION_RECURSE,
						fileAction, dirAction, userData, depth+1)) {
				status = 0;
			}
			free(nextFile);
		}
	}

	if (DO_SORT(flags)) {
		list_sort(NULL, &dirs, &cmp_dirlist);

		list_for_each_entry_safe(entry, entry_tmp, &dirs, list){
			/* descend into it, forcing recursion. */
			if (!recursive_action(entry->dirname, flags | ACTION_RECURSE,
						fileAction, dirAction, userData, depth+1)) {
				status = 0;
			}

			list_del(&entry->list);
			free(entry->dirname);
		}
	}
	closedir(dir);
	if ((flags & ACTION_DEPTHFIRST) &&
		!dirAction(fileName, &statbuf, userData, depth)) {
			goto done_nak_warn;
	}

	if (!status)
		return 0;
	return 1;
done_nak_warn:
	return 0;
}

#ifdef __BAREBOX__
EXPORT_SYMBOL(recursive_action);
#endif
