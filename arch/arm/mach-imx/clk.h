#ifndef __IMX_CLK_H
#define __IMX_CLK_H

static inline struct clk *imx_clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, reg, shift, width);
}

static inline struct clk *imx_clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div)
{
	return clk_fixed_factor(name, parent, mult, div);
}

static inline struct clk *imx_clk_mux(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, reg, shift, width, parents, num_parents);
}

static inline struct clk *imx_clk_gate(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate(name, parent, reg, shift);
}

struct clk *imx_clk_pllv1(const char *name, const char *parent,
		void __iomem *base);

struct clk *imx_clk_pllv2(const char *name, const char *parent,
		void __iomem *base);

enum imx_pllv3_type {
	IMX_PLLV3_GENERIC,
	IMX_PLLV3_SYS,
	IMX_PLLV3_USB,
	IMX_PLLV3_AV,
	IMX_PLLV3_ENET,
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

#endif /* __IMX_CLK_H */
