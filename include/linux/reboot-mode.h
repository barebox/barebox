/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __REBOOT_MODE_H__
#define __REBOOT_MODE_H__

#include <linux/types.h>

struct device_d;

#ifdef CONFIG_REBOOT_MODE
struct reboot_mode_driver {
	struct device_d *dev;
	int (*write)(struct reboot_mode_driver *reboot, const u32 *magic);
	int priority;
	bool no_fixup;

	/* filled by reboot_mode_register */
	int reboot_mode_prev, reboot_mode_next;
	unsigned nmodes, nelems;
	const char **modes;
	u32 *magics;
};

int reboot_mode_register(struct reboot_mode_driver *reboot,
			 const u32 *magic, size_t num);
const char *reboot_mode_get(void);

#define REBOOT_MODE_DEFAULT_PRIORITY 100

#else

static inline const char *reboot_mode_get(void)
{
	return NULL;
}

#endif

#endif
