/* SPDX-License-Identifier: GPL-2.0 */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/clk/meson/clk-dualdiv.h?id=889c2b7ec42b8d14d421541f0a3ae1238e34891e */
/*
 * Copyright (c) 2019 BayLibre, SAS.
 * Author: Jerome Brunet <jbrunet@baylibre.com>
 */

#ifndef __MESON_CLK_DUALDIV_H
#define __MESON_CLK_DUALDIV_H

#include <linux/clk-provider.h>
#include "parm.h"

struct meson_clk_dualdiv_param {
	unsigned int n1;
	unsigned int n2;
	unsigned int m1;
	unsigned int m2;
	unsigned int dual;
};

struct meson_clk_dualdiv_data {
	struct parm n1;
	struct parm n2;
	struct parm m1;
	struct parm m2;
	struct parm dual;
	const struct meson_clk_dualdiv_param *table;
};

extern const struct clk_ops meson_clk_dualdiv_ops;
extern const struct clk_ops meson_clk_dualdiv_ro_ops;

#endif /* __MESON_CLK_DUALDIV_H */
