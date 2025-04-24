/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTARGS_H
#define __BOOTARGS_H

#include <environment.h>

extern int linux_rootwait_secs;

#ifdef CONFIG_FLEXIBLE_BOOTARGS
const char *linux_bootargs_get(void);
int linux_bootargs_overwrite(const char *bootargs);
char *linux_bootargs_append_rootwait(char *rootarg_old);
#else
static inline const char *linux_bootargs_get(void)
{
	return getenv("bootargs");
}

static inline int linux_bootargs_overwrite(const char *bootargs)
{
	return setenv("bootargs", bootargs);
}
static inline char *linux_bootargs_append_rootwait(char *rootarg)
{
	return rootarg;
}
#endif

#endif /* __BOOTARGS_H */
