/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MACH_MESON_MESON_H
#define __MACH_MESON_MESON_H

#include <linux/kconfig.h>
#include <linux/types.h>

enum meson_soc {
	MESON_SOC_UNKNOWN,
	MESON_SOC_G12,
};

/* the SoC family barebox runs on, as the device tree says */
enum meson_soc meson_soc(void);

static inline bool meson_is_g12(void)
{
	return IS_ENABLED(CONFIG_ARCH_MESON_G12) &&
	       meson_soc() == MESON_SOC_G12;
}

#endif
