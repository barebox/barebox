/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DEVBOOT_H
#define __DEVBOOT_H

#include <linux/stddef.h>
#include <linux/types.h>

struct bootm_overrides;
struct devboot;

#ifdef CONFIG_BOOT_DEVBOOT
bool devboot_enabled(void);
struct devboot *devboot_start(struct bootm_overrides *overrides);
void devboot_finish(struct devboot *db);
#else
static inline bool devboot_enabled(void)
{
	return false;
}

static inline struct devboot *devboot_start(struct bootm_overrides *overrides)
{
	return NULL;
}

static inline void devboot_finish(struct devboot *db)
{
}
#endif

#endif
