/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MACH_MESON_MESON_H
#define __MACH_MESON_MESON_H

#include <linux/kconfig.h>
#include <linux/types.h>

enum meson_soc {
	MESON_SOC_UNKNOWN,
	MESON_SOC_GX,
	MESON_SOC_G12,
	MESON_SOC_A9,
};

/* the SoC family barebox runs on, as the device tree says */
enum meson_soc meson_soc(void);

static inline bool meson_is_gx(void)
{
	return IS_ENABLED(CONFIG_ARCH_MESON_GX) &&
	       meson_soc() == MESON_SOC_GX;
}

static inline bool meson_is_g12(void)
{
	return IS_ENABLED(CONFIG_ARCH_MESON_G12) &&
	       meson_soc() == MESON_SOC_G12;
}

static inline bool meson_is_a9(void)
{
	return IS_ENABLED(CONFIG_ARCH_MESON_A9) &&
	       meson_soc() == MESON_SOC_A9;
}

#endif
