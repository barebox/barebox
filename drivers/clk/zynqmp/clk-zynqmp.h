/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016-2018 Xilinx
 */

#ifndef __LINUX_CLK_ZYNQMP_H_
#define __LINUX_CLK_ZYNQMP_H_

enum topology_type {
	TYPE_INVALID,
	TYPE_MUX,
	TYPE_PLL,
	TYPE_FIXEDFACTOR,
	TYPE_DIV1,
	TYPE_DIV2,
	TYPE_GATE,
};

struct clock_topology {
	enum topology_type type;
	u32 flag;
	u32 type_flag;
};

struct clk *zynqmp_clk_register_pll(const char *name,
				    unsigned int clk_id,
				    const char **parents,
				    unsigned int num_parents,
				    const struct clock_topology *node);

struct clk *zynqmp_clk_register_gate(const char *name,
				     unsigned int clk_id,
				     const char **parents,
				     unsigned int num_parents,
				     const struct clock_topology *node);

struct clk *zynqmp_clk_register_divider(const char *name,
					unsigned int clk_id,
					const char **parents,
					unsigned int num_parents,
					const struct clock_topology *node);

struct clk *zynqmp_clk_register_mux(const char *name,
				    unsigned int clk_id,
				    const char **parents,
				    unsigned int num_parents,
				    const struct clock_topology *node);

struct clk *zynqmp_clk_register_fixed_factor(const char *name,
					     unsigned int clk_id,
					     const char **parents,
					     unsigned int num_parents,
					     const struct clock_topology *node);

#endif
