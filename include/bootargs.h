/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTARGS_H
#define __BOOTARGS_H

#include <environment.h>

#ifdef CONFIG_FLEXIBLE_BOOTARGS
const char *linux_bootargs_get(void);
int linux_bootargs_overwrite(const char *bootargs);
#else
static inline const char *linux_bootargs_get(void)
{
	return getenv("bootargs");
}

static inline int linux_bootargs_overwrite(const char *bootargs)
{
	return setenv("bootargs", bootargs);
}
#endif

#endif /* __BOOTARGS_H */
