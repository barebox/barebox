/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* struct tegra_clk_frac_div - fractional divider */
struct tegra_clk_frac_div {
	struct clk	hw;
	void __iomem	*reg;
	u8		flags;
	u8		shift;
	u8		width;
	u8		frac_width;
	const char	*parent;
};

#define TEGRA_DIVIDER_ROUND_UP	BIT(0)
#define TEGRA_DIVIDER_FIXED	BIT(1)
#define TEGRA_DIVIDER_INT	BIT(2)
#define TEGRA_DIVIDER_UART	BIT(3)

struct clk *tegra_clk_divider_alloc(const char *name, const char *parent_name,
		void __iomem *reg, unsigned long flags, u8 clk_divider_flags,
		u8 shift, u8 width, u8 frac_width);

void tegra_clk_divider_free(struct clk *divider);

struct clk *tegra_clk_register_divider(const char *name,
		const char *parent_name, void __iomem *reg, unsigned long flags,
		u8 clk_divider_flags, u8 shift, u8 width, u8 frac_width);

/* struct tegra_clk_pll_freq_table - PLL frequecy table */
struct tegra_clk_pll_freq_table {
	unsigned long	input_rate;
	unsigned long	output_rate;
	u16		n;
	u16		m;
	u8		p;
	u8		cpcon;
};

/* struct clk_pll_params - PLL parameters */
struct tegra_clk_pll_params {
	unsigned long	input_min;
	unsigned long	input_max;
	unsigned long	cf_min;
	unsigned long	cf_max;
	unsigned long	vco_min;
	unsigned long	vco_max;

	u32		base_reg;
	u32		misc_reg;
	u32		lock_reg;
	u8		lock_bit_idx;
	u8		lock_enable_bit_idx;
	int		lock_delay;
};

/* struct tegra_clk_pll - Tegra PLL clock */
struct tegra_clk_pll {
	struct clk	hw;
	void __iomem	*clk_base;
	u8		flags;
	unsigned long	fixed_rate;
	u8		divn_shift;
	u8		divn_width;
	u8		divm_shift;
	u8		divm_width;
	u8		divp_shift;
	u8		divp_width;
	struct tegra_clk_pll_freq_table	*freq_table;
	struct tegra_clk_pll_params	*params;
	const char	*parent;
};

#define TEGRA_PLL_USE_LOCK	BIT(0)
#define TEGRA_PLL_HAS_CPCON	BIT(1)
#define TEGRA_PLL_SET_LFCON	BIT(2)
#define TEGRA_PLL_SET_DCCON	BIT(3)
#define TEGRA_PLLU		BIT(4)
#define TEGRA_PLLM		BIT(5)
#define TEGRA_PLL_FIXED		BIT(6)
#define TEGRA_PLLE_CONFIGURE	BIT(7)

struct clk *tegra_clk_register_pll(const char *name, const char *parent_name,
		void __iomem *clk_base,
		unsigned long flags, unsigned long fixed_rate,
		struct tegra_clk_pll_params *pll_params, u8 pll_flags,
		struct tegra_clk_pll_freq_table *freq_table);

/* struct tegra_clk_pll_out - PLL output divider */
struct tegra_clk_pll_out {
	struct clk	hw;
	struct clk	*div;
	void __iomem	*reg;
	u8		enb_bit_idx;
	u8		rst_bit_idx;
	const char	*parent;
};

struct clk *tegra_clk_register_pll_out(const char *name,
		const char *parent_name, void __iomem *reg, u8 shift,
		u8 divider_flags);

/* struct clk-periph - peripheral clock */
struct tegra_clk_periph {
	struct clk	hw;
	struct clk	*gate;
	struct clk	*mux;
	struct clk	*div;
	u8 		flags;
	u8		rst_shift;
	void __iomem	*rst_reg;
};

#define TEGRA_PERIPH_NO_RESET BIT(0)
#define TEGRA_PERIPH_MANUAL_RESET BIT(1)
#define TEGRA_PERIPH_ON_APB BIT(2)

struct clk *tegra_clk_register_periph_nodiv(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags);

struct clk *tegra_clk_register_periph(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags);

/* struct clk_init_table - clock initialization table */
struct tegra_clk_init_table {
	unsigned int	clk_id;
	unsigned int	parent_id;
	unsigned long	rate;
	int		state;
};

void tegra_init_from_table(struct tegra_clk_init_table *tbl,
		struct clk *clks[], int clk_max);
