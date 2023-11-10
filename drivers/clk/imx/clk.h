/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __IMX_CLK_H
#define __IMX_CLK_H

struct clk *clk_gate2(const char *name, const char *parent, void __iomem *reg,
		      u8 shift, u8 cgr_val, unsigned long flags);

static inline struct clk *imx_clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, CLK_SET_RATE_PARENT, reg, shift, width,
			   0);
}

static inline struct clk *imx_clk_divider_flags(const char *name,
                const char *parent, void __iomem *reg, u8 shift, u8 width,
                unsigned long flags)
{
	return clk_divider(name, parent, flags, reg, shift, width, 0);
}

static inline struct clk *imx_clk_divider_np(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, 0, reg, shift, width, 0);
}

static inline struct clk *imx_clk_divider2(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, CLK_OPS_PARENT_ENABLE, reg, shift,
			   width, 0);
}

static inline struct clk *imx_clk_divider_table(const char *name,
		const char *parent, void __iomem *reg, u8 shift, u8 width,
		const struct clk_div_table *table)
{
	return clk_divider_table(name, parent, CLK_SET_RATE_PARENT, reg, shift,
				 width, table, 0);
}

static inline struct clk *__imx_clk_mux(const char *name, void __iomem *reg,
					u8 shift, u8 width, const char * const *parents,
					u8 num_parents, unsigned flags, unsigned long clk_mux_flags)
{
	return clk_mux(name, CLK_SET_RATE_NO_REPARENT | flags, reg,
		       shift, width, parents, num_parents, clk_mux_flags);
}

static inline struct clk *imx_clk_mux_ldb(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char * const *parents, int num_parents)
{
	return __imx_clk_mux(name, reg, shift, width, parents, num_parents,
			     CLK_SET_RATE_PARENT, CLK_MUX_READ_ONLY);
}


static inline struct clk *imx_clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div)
{
	return clk_fixed_factor(name, parent, mult, div, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_mux_flags(const char *name, void __iomem *reg,
					    u8 shift, u8 width,
					    const char * const *parents, u8 num_parents,
					    unsigned long clk_flags)
{
	return __imx_clk_mux(name, reg, shift, width, parents, num_parents,
			     clk_flags, 0);
}

static inline struct clk *imx_clk_mux2_flags(const char *name,
		void __iomem *reg, u8 shift, u8 width, const char * const *parents,
		int num_parents, unsigned long clk_flags)
{
	return __imx_clk_mux(name,reg, shift, width, parents, num_parents,
			     clk_flags | CLK_OPS_PARENT_ENABLE, 0);
}

static inline struct clk *imx_clk_mux(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char * const *parents, u8 num_parents)
{
	return __imx_clk_mux(name, reg, shift, width, parents, num_parents, 0, 0);
}

static inline struct clk *imx_clk_mux2(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char * const *parents, u8 num_parents)
{
	return __imx_clk_mux(name, reg, shift, width, parents,
			     num_parents, CLK_OPS_PARENT_ENABLE, 0);
}

static inline struct clk *imx_clk_mux_p(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char * const *parents, u8 num_parents)
{
	return __imx_clk_mux(name, reg, shift, width, parents, num_parents,
			     CLK_SET_RATE_PARENT,  0);
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

static inline struct clk *imx_clk_gate2_shared2(const char *name, const char *parent,
						void __iomem *reg, u8 shift)
{
	return clk_gate2(name, parent, reg, shift, 0x3,
			 CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);
}

static inline struct clk *imx_clk_gate2_flags(const char *name,
		const char *parent, void __iomem *reg, u8 shift,
		unsigned long flags)
{
	return clk_gate2(name, parent, reg, shift, 0x3, flags);
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

static inline struct clk *imx_clk_gate4_flags(const char *name, const char *parent,
		void __iomem *reg, u8 shift, unsigned long flags)
{
	return clk_gate2(name, parent, reg, shift, 0x3, flags | CLK_OPS_PARENT_ENABLE);
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

enum imx_pll14xx_type {
	PLL_1416X,
	PLL_1443X,
};

/* NOTE: Rate table should be kept sorted in descending order. */
struct imx_pll14xx_rate_table {
	unsigned int rate;
	unsigned int pdiv;
	unsigned int mdiv;
	unsigned int sdiv;
	unsigned int kdiv;
};

#define PLL_1416X_RATE(_rate, _m, _p, _s)		\
	{						\
		.rate	=	(_rate),		\
		.mdiv	=	(_m),			\
		.pdiv	=	(_p),			\
		.sdiv	=	(_s),			\
	}

#define PLL_1443X_RATE(_rate, _m, _p, _s, _k)		\
	{						\
		.rate	=	(_rate),		\
		.mdiv	=	(_m),			\
		.pdiv	=	(_p),			\
		.sdiv	=	(_s),			\
		.kdiv	=	(_k),			\
	}

struct imx_pll14xx_clk {
	enum imx_pll14xx_type type;
	const struct imx_pll14xx_rate_table *rate_table;
	int rate_count;
	int flags;
};

extern struct imx_pll14xx_clk imx_1443x_pll;
extern struct imx_pll14xx_clk imx_1416x_pll;

struct clk *imx_clk_pll14xx(const char *name, const char *parent_name,
                 void __iomem *base, const struct imx_pll14xx_clk *pll_clk);

struct clk *imx_clk_frac_pll(const char *name, const char *parent_name,
			     void __iomem *base);

enum imx_sccg_pll_type {
	SCCG_PLL1,
	SCCG_PLL2,
};

struct clk *imx_clk_sccg_pll(const char *name, const char *parent_name,
			     void __iomem *base,
			     enum imx_sccg_pll_type pll_type);

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

#define IMX_COMPOSITE_CORE      BIT(0)
#define IMX_COMPOSITE_BUS       BIT(1)

struct clk *imx8m_clk_composite_flags(const char *name,
		const char * const *parent_names, int num_parents, void __iomem *reg,
		u32 composite_flags,
		unsigned long flags);

#define imx8m_clk_hw_composite_core(name, parent_names, reg)    \
        imx8m_clk_hw_composite_flags(name, parent_names, \
                        ARRAY_SIZE(parent_names), reg, \
                        IMX_COMPOSITE_CORE, \
                        CLK_SET_RATE_NO_REPARENT | CLK_OPS_PARENT_ENABLE)

#define imx8m_clk_hw_composite_bus(name, parent_names, reg)     \
        imx8m_clk_hw_composite_flags(name, parent_names, \
                        ARRAY_SIZE(parent_names), reg, \
                        IMX_COMPOSITE_BUS, \
                        CLK_SET_RATE_NO_REPARENT | CLK_OPS_PARENT_ENABLE)

#define __imx8m_clk_composite(name, parent_names, reg, flags) \
		imx8m_clk_composite_flags(name, parent_names, \
			ARRAY_SIZE(parent_names), reg, 0, \
			flags | CLK_OPS_PARENT_ENABLE)

#define imx8m_clk_composite(name, parent_names, reg) \
	__imx8m_clk_composite(name, parent_names, reg, 0)

#define imx8m_clk_composite_critical(name, parent_names, reg) \
	__imx8m_clk_composite(name, parent_names, reg, CLK_IS_CRITICAL)

#include <soc/imx/clk-fracn-gppll.h>

struct clk *imx93_clk_composite_flags(const char *name,
				      const char * const *parent_names,
				      int num_parents,
				      void __iomem *reg,
				      u32 domain_id,
				      unsigned long flags);
#define imx93_clk_composite(name, parent_names, num_parents, reg, domain_id) \
        imx93_clk_composite_flags(name, parent_names, num_parents, reg, domain_id \
                                  CLK_SET_RATE_NO_REPARENT | CLK_OPS_PARENT_ENABLE)

struct clk *imx93_clk_gate(struct device *dev, const char *name, const char *parent_name,
			   unsigned long flags, void __iomem *reg, u32 bit_idx, u32 val,
			   u32 mask, u32 domain_id, unsigned int *share_count);

/*
 * Names of the above functions used in the Linux Kernel. Added here
 * to be able to use the same names in barebox to reduce the diffs
 * between barebox and Linux clk drivers.
 */
#define imx_clk_hw_mux imx_clk_mux
#define imx_clk_hw_pll14xx imx_clk_pll14xx
#define imx_clk_hw_gate imx_clk_gate
#define imx_clk_hw_fixed_factor imx_clk_fixed_factor
#define imx_clk_hw_mux_flags imx_clk_mux_flags
#define imx_clk_hw_divider2 imx_clk_divider2
#define imx_clk_hw_mux2_flags imx_clk_mux2_flags
#define imx_clk_hw_gate4_flags imx_clk_gate4_flags
#define imx_clk_hw_gate4 imx_clk_gate4
#define imx_clk_hw_cpu imx_clk_cpu
#define imx_clk_hw_gate2 imx_clk_gate2
#define imx8m_clk_hw_composite_flags imx8m_clk_composite_flags
#define imx8m_clk_hw_composite imx8m_clk_composite
#define imx8m_clk_hw_composite_critical imx8m_clk_composite_critical
#define imx_clk_hw_gate2_shared2 imx_clk_gate2_shared2
#define imx_clk_hw_mux2 imx_clk_mux2

#endif /* __IMX_CLK_H */
