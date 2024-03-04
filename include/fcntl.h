/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __FCNTL_H
#define __FCNTL_H

#include <linux/types.h>

/* open/fcntl - O_SYNC is only implemented on blocks devices and on files
   located on an ext2 file system */
#define O_ACCMODE	00000003
#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100	/* not fcntl */
#define O_EXCL		00000200	/* not fcntl */
#define O_TRUNC		00001000	/* not fcntl */
#define O_APPEND	00002000
#define O_DIRECTORY	00200000	/* must be a directory */
#define O_NOFOLLOW	00400000	/* don't follow links */
#define __O_TMPFILE	020000000

#define O_TMPFILE       (__O_TMPFILE | O_DIRECTORY)

/* barebox additional flags */
#define O_RWSIZE_MASK	017000000
#define O_RWSIZE_SHIFT	18
#define O_RWSIZE_1	001000000
#define O_RWSIZE_2	002000000
#define O_RWSIZE_4	004000000
#define O_RWSIZE_8	010000000

int open(const char *pathname, int flags, ...);

static inline int creat(const char *pathname, mode_t mode)
{
	return open(pathname, O_CREAT | O_WRONLY | O_TRUNC);
}

#endif /* __FCNTL_H */
