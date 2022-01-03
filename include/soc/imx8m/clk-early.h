/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IMX8M_CLK_EARLY_H
#define __SOC_IMX8M_CLK_EARLY_H

int clk_pll1416x_early_set_rate(void __iomem *base, unsigned long drate,
			  unsigned long prate);

#endif /* __SOC_IMX8M_CLK_EARLY_H */
