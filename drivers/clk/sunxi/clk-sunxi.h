/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2024 Jules Maselbas */
#include <linux/clk.h>

static inline struct clk *
sunxi_clk_gate(const char *name, const char *parent, void __iomem *reg, u8 shift)
{
	return clk_gate(name, parent, reg, shift, 0, 0);
}

static inline struct clk *
sunxi_clk_mux(const char *name, const char * const *parents, u8 num_parents,
	      void __iomem *reg, u8 shift, u8 width)
{
	return clk_mux(name, 0, reg, shift, width, parents, num_parents, 0);
}

static inline struct clk *
sunxi_clk_div_table(const char *name, const char *parent, struct clk_div_table *table,
	      void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider_table(name, parent, CLK_SET_RATE_PARENT, reg, shift,
				 width, table, 0);
}

static inline struct clk *
sunxi_clk_div(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, CLK_SET_RATE_PARENT, reg, shift,
				 width, 0);
}

static inline void nop_delay(volatile u32 cnt)
{
	while (cnt--)
		barrier();
}

/* a "one size fit all" clk for barebox */

struct ccu_clk {
	struct clk_hw	hw;
	u32 reg;
	void __iomem	*base;

	u32 gate;

	struct ccu_clk_mux {
		u8		shift;
		u8		width;
		const u32	*table;
	} mux;

	u8 num_div;
	struct ccu_clk_div {
		u8		shift;
		u8		width;
		u8		type;
		u16		max;
		u16		off;
	} div[4];
};

enum ccu_clk_div_type {
	CCU_CLK_DIV_FIXED = 1,
	CCU_CLK_DIV_M,
	CCU_CLK_DIV_P,
};

extern const struct clk_ops ccu_clk_ops;

struct clk *sunxi_ccu_clk(const char *name,
			  const char * const *parents, u8 num_parents,
			  void __iomem *base, u32 reg,
			  struct ccu_clk_mux mux,
			  const struct ccu_clk_div *div, u8 num_div,
			  u32 gate,
			  int flags);
