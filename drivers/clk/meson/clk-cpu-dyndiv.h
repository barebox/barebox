/* SPDX-License-Identifier: GPL-2.0 */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/clk/meson/clk-cpu-dyndiv.h?id=26d34431add04a98a60b8935c25765914fa773f7 */
/*
 * Copyright (c) 2019 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#ifndef __MESON_CLK_CPU_DYNDIV_H
#define __MESON_CLK_CPU_DYNDIV_H

#include <linux/clk-provider.h>
#include "parm.h"

struct meson_clk_cpu_dyndiv_data {
	struct parm div;
	struct parm dyn;
};

extern const struct clk_ops meson_clk_cpu_dyndiv_ops;

#endif /* __MESON_CLK_CPU_DYNDIV_H */
