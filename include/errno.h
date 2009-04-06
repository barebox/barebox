#ifndef __ERRNO_H
#define __ERRNO_H

#include <asm-generic/errno.h>

extern int errno;

void perror(const char *s);
const char *errno_str(void);
const char *strerror(int errnum);

#endif /* __ERRNO_H */
