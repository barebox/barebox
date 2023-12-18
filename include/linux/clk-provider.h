/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Copyright (c) 2010-2011 Jeremy Kerr <jeremy.kerr@canonical.com>
 *  Copyright (C) 2011-2012 Linaro Ltd <mturquette@linaro.org>
 */
#ifndef __LINUX_CLK_PROVIDER_H
#define __LINUX_CLK_PROVIDER_H

#include <linux/clk.h>

long divider_round_rate_parent(struct clk_hw *hw, struct clk_hw *parent,
			       unsigned long rate, unsigned long *prate,
			       const struct clk_div_table *table,
			       u8 width, unsigned long flags);

long divider_ro_round_rate_parent(struct clk_hw *hw, struct clk_hw *parent,
				  unsigned long rate, unsigned long *prate,
				  const struct clk_div_table *table, u8 width,
				  unsigned long flags, unsigned int val);

static inline long divider_ro_round_rate(struct clk_hw *hw, unsigned long rate,
					 unsigned long *prate,
					 const struct clk_div_table *table,
					 u8 width, unsigned long flags,
					 unsigned int val)
{
	return divider_ro_round_rate_parent(hw, clk_hw_get_parent(hw),
					    rate, prate, table, width, flags,
					    val);
}

/**
 * struct clk_rate_request - Structure encoding the clk constraints that
 * a clock user might require.
 *
 * Should be initialized by calling clk_hw_init_rate_request().
 *
 * @core: 		Pointer to the struct clk_core affected by this request
 * @rate:		Requested clock rate. This field will be adjusted by
 *			clock drivers according to hardware capabilities.
 * @min_rate:		Minimum rate imposed by clk users.
 * @max_rate:		Maximum rate imposed by clk users.
 * @best_parent_rate:	The best parent rate a parent can provide to fulfill the
 *			requested constraints.
 * @best_parent_hw:	The most appropriate parent clock that fulfills the
 *			requested constraints.
 *
 */
struct clk_rate_request {
	struct clk_core *core;
	unsigned long rate;
	unsigned long min_rate;
	unsigned long max_rate;
	unsigned long best_parent_rate;
	struct clk_hw *best_parent_hw;
};

#define CLK_HW_INIT(_name, _parent, _ops, _flags)		\
	(&(struct clk_init_data) {				\
		.flags		= _flags,			\
		.name		= _name,			\
		.parent_names	= (const char *[]) { _parent },	\
		.num_parents	= 1,				\
		.ops		= _ops,				\
	})

#define CLK_HW_INIT_HW(_name, _parent, _ops, _flags)			\
	(&(struct clk_init_data) {					\
		.flags		= _flags,				\
		.name		= _name,				\
		.parent_hws	= (const struct clk_hw*[]) { _parent },	\
		.num_parents	= 1,					\
		.ops		= _ops,					\
	})

/*
 * This macro is intended for drivers to be able to share the otherwise
 * individual struct clk_hw[] compound literals created by the compiler
 * when using CLK_HW_INIT_HW. It does NOT support multiple parents.
 */
#define CLK_HW_INIT_HWS(_name, _parent, _ops, _flags)			\
	(&(struct clk_init_data) {					\
		.flags		= _flags,				\
		.name		= _name,				\
		.parent_hws	= _parent,				\
		.num_parents	= 1,					\
		.ops		= _ops,					\
	})

#define CLK_HW_INIT_FW_NAME(_name, _parent, _ops, _flags)		\
	(&(struct clk_init_data) {					\
		.flags		= _flags,				\
		.name		= _name,				\
		.parent_data	= (const struct clk_parent_data[]) {	\
					{ .fw_name = _parent },		\
				  },					\
		.num_parents	= 1,					\
		.ops		= _ops,					\
	})

#define CLK_HW_INIT_PARENTS(_name, _parents, _ops, _flags)	\
	(&(struct clk_init_data) {				\
		.flags		= _flags,			\
		.name		= _name,			\
		.parent_names	= _parents,			\
		.num_parents	= ARRAY_SIZE(_parents),		\
		.ops		= _ops,				\
	})

#define CLK_HW_INIT_PARENTS_HW(_name, _parents, _ops, _flags)	\
	(&(struct clk_init_data) {				\
		.flags		= _flags,			\
		.name		= _name,			\
		.parent_hws	= _parents,			\
		.num_parents	= ARRAY_SIZE(_parents),		\
		.ops		= _ops,				\
	})

#define CLK_HW_INIT_PARENTS_DATA(_name, _parents, _ops, _flags)	\
	(&(struct clk_init_data) {				\
		.flags		= _flags,			\
		.name		= _name,			\
		.parent_data	= _parents,			\
		.num_parents	= ARRAY_SIZE(_parents),		\
		.ops		= _ops,				\
	})

#define CLK_HW_INIT_NO_PARENT(_name, _ops, _flags)	\
	(&(struct clk_init_data) {			\
		.flags          = _flags,		\
		.name           = _name,		\
		.parent_names   = NULL,			\
		.num_parents    = 0,			\
		.ops            = _ops,			\
	})

#define CLK_FIXED_FACTOR(_struct, _name, _parent,			\
			_div, _mult, _flags)				\
	struct clk_fixed_factor _struct = {				\
		.div		= _div,					\
		.mult		= _mult,				\
		.hw.init	= CLK_HW_INIT(_name,			\
					      _parent,			\
					      &clk_fixed_factor_ops,	\
					      _flags),			\
	}

#define CLK_FIXED_FACTOR_HW(_struct, _name, _parent,			\
			    _div, _mult, _flags)			\
	struct clk_fixed_factor _struct = {				\
		.div		= _div,					\
		.mult		= _mult,				\
		.hw.init	= CLK_HW_INIT_HW(_name,			\
						 _parent,		\
						 &clk_fixed_factor_ops,	\
						 _flags),		\
	}

/*
 * This macro allows the driver to reuse the _parent array for multiple
 * fixed factor clk declarations.
 */
#define CLK_FIXED_FACTOR_HWS(_struct, _name, _parent,			\
			     _div, _mult, _flags)			\
	struct clk_fixed_factor _struct = {				\
		.div		= _div,					\
		.mult		= _mult,				\
		.hw.init	= CLK_HW_INIT_HWS(_name,		\
						  _parent,		\
						  &clk_fixed_factor_ops, \
						  _flags),	\
	}

#define CLK_FIXED_FACTOR_FW_NAME(_struct, _name, _parent,		\
				 _div, _mult, _flags)			\
	struct clk_fixed_factor _struct = {				\
		.div		= _div,					\
		.mult		= _mult,				\
		.hw.init	= CLK_HW_INIT_FW_NAME(_name,		\
						      _parent,		\
						      &clk_fixed_factor_ops, \
						      _flags),		\
	}

#endif
