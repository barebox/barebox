/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __STAT_H
#define __STAT_H

#include <linux/types.h>
#include <linux/stat.h>
#include <fcntl.h>

int mkdirat(int dirfd, const char *pathname, mode_t mode);

static inline int mkdir(const char *pathname, mode_t mode)
{
	return mkdirat(AT_FDCWD, pathname, mode);
}

#endif /* __STAT_H */
