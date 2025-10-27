/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ERRNO_H
#define __ERRNO_H

#include <linux/errno.h>
#include <linux/err.h>

extern int errno;

#if IN_PROPER
void perror(const char *s);
const char *strerror(int errnum);
#else
static inline void perror(const char *s)
{
}
static inline const char *strerror(int errnum)
{
	return "unknown error";
}
#endif

static inline int errno_set(int err)
{
	if (err < 0)
		errno = -err;
	return err;
}

static inline int errno_setp(const void *errp)
{
	int error = IS_ERR(errp) ? PTR_ERR(errp) : 0;
	return errno_set(error);
}

#endif /* __ERRNO_H */
