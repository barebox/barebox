#ifndef __IMX_CLK_H
#define __IMX_CLK_H

struct clk *clk_gate2(const char *name, const char *parent, void __iomem *reg,
		      u8 shift, u8 cgr_val, unsigned long flags);

static inline struct clk *imx_clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, reg, shift, width, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_divider_np(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, reg, shift, width, 0);
}

static inline struct clk *imx_clk_divider2(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, reg, shift, width, CLK_OPS_PARENT_ENABLE);
}

static inline struct clk *imx_clk_divider_table(const char *name,
		const char *parent, void __iomem *reg, u8 shift, u8 width,
		const struct clk_div_table *table)
{
	return clk_divider_table(name, parent, reg, shift, width, table,
			CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div)
{
	return clk_fixed_factor(name, parent, mult, div, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_mux_flags(const char *name, void __iomem *reg,
					    u8 shift, u8 width,
					    const char **parents, u8 num_parents,
					    unsigned long flags)
{
	return clk_mux(name, reg, shift, width, parents, num_parents, flags);
}

static inline struct clk *imx_clk_mux(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, reg, shift, width, parents, num_parents, 0);
}

static inline struct clk *imx_clk_mux2(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, reg, shift, width, parents, num_parents, CLK_OPS_PARENT_ENABLE);
}

static inline struct clk *imx_clk_mux_p(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, reg, shift, width, parents, num_parents, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_gate(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate(name, parent, reg, shift, CLK_SET_RATE_PARENT, 0);
}

static inline struct clk *imx_clk_gate_dis(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate_inverted(name, parent, reg, shift, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_gate2(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate2(name, parent, reg, shift, 0x3, 0);
}

static inline struct clk *imx_clk_gate2_cgr(const char *name, const char *parent,
					    void __iomem *reg, u8 shift, u8 cgr_val)
{
	return clk_gate2(name, parent, reg, shift, cgr_val, 0);
}

static inline struct clk *imx_clk_gate3(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate(name, parent, reg, shift, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE, 0);
}

static inline struct clk *imx_clk_gate4(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate2(name, parent, reg, shift, 0x3, CLK_OPS_PARENT_ENABLE);
}

static inline struct clk *imx_clk_gate_shared(const char *name, const char *parent,
					      const char *shared)
{
	return clk_gate_shared(name, parent, shared, CLK_SET_RATE_PARENT);
}

struct clk *imx_clk_pllv1(const char *name, const char *parent,
		void __iomem *base);

struct clk *imx_clk_pllv2(const char *name, const char *parent,
		void __iomem *base);

enum imx_pllv3_type {
	IMX_PLLV3_GENERIC,
	IMX_PLLV3_SYS,
	IMX_PLLV3_SYS_VF610,
	IMX_PLLV3_USB,
	IMX_PLLV3_USB_VF610,
	IMX_PLLV3_AV,
	IMX_PLLV3_ENET,
	IMX_PLLV3_ENET_IMX7,
	IMX_PLLV3_MLB,
};

struct clk *imx_clk_pllv3(enum imx_pllv3_type type, const char *name,
			  const char *parent, void __iomem *base,
			  u32 div_mask);

struct clk *imx_clk_pfd(const char *name, const char *parent,
			void __iomem *reg, u8 idx);

static inline struct clk *imx_clk_busy_divider(const char *name, const char *parent,
				 void __iomem *reg, u8 shift, u8 width,
				 void __iomem *busy_reg, u8 busy_shift)
{
	/*
	 * For now we do not support rate setting, so just fall back to
	 * regular divider.
	 */
	return imx_clk_divider(name, parent, reg, shift, width);
}

static inline struct clk *imx_clk_busy_mux(const char *name, void __iomem *reg, u8 shift,
			     u8 width, void __iomem *busy_reg, u8 busy_shift,
			     const char **parents, int num_parents)
{
	/*
	 * For now we do not support mux switching, so just fall back to
	 * regular mux.
	 */
	return imx_clk_mux(name, reg, shift, width, parents, num_parents);
}

struct clk *imx_clk_gate_exclusive(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u32 exclusive_mask);

void imx_check_clocks(struct clk *clks[], unsigned int count);

struct clk *imx_clk_cpu(const char *name, const char *parent_name,
		struct clk *div, struct clk *mux, struct clk *pll,
		struct clk *step);

#endif /* __IMX_CLK_H */
