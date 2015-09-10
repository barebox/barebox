#ifndef __ERRNO_H
#define __ERRNO_H

#include <asm-generic/errno.h>
#include <linux/err.h>

extern int errno;

void perror(const char *s);
const char *errno_str(void);
const char *strerror(int errnum);

static inline const char *strerrorp(const void *errp)
{
	return strerror(-PTR_ERR(errp));
}

#endif /* __ERRNO_H */
