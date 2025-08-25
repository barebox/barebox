/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _CLK_FIXED_H
#define _CLK_FIXED_H

#include <linux/types.h>

struct clk;

bool clk_is_fixed(struct clk *clk);

#endif
