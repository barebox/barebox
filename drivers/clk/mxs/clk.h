#ifndef __MXS_CLK_H
#define __MXS_CLK_H

int mxs_clk_wait(void __iomem *reg, u8 shift);

struct clk *mxs_clk_pll(const char *name, const char *parent_name,
			void __iomem *base, u8 power, unsigned long rate);

struct clk *mxs_clk_ref(const char *name, const char *parent_name,
			void __iomem *reg, u8 idx);

struct clk *mxs_clk_div(const char *name, const char *parent_name,
			void __iomem *reg, u8 shift, u8 width, u8 busy);

struct clk *mxs_clk_frac(const char *name, const char *parent_name,
			 void __iomem *reg, u8 shift, u8 width, u8 busy);

#ifdef CONFIG_DRIVER_VIDEO_STM
struct clk *mxs_clk_lcdif(const char *name, struct clk *frac, struct clk *div,
			  struct clk *gate);
#else
static inline struct clk *mxs_clk_lcdif(const char *name, struct clk *frac, struct clk *div,
			  struct clk *gate)
{
	return ERR_PTR(-ENOSYS);
}
#endif

static inline struct clk *mxs_clk_fixed(const char *name, int rate)
{
	return clk_fixed(name, rate);
}

static inline struct clk *mxs_clk_gate(const char *name,
			const char *parent_name, void __iomem *reg, u8 shift)
{
	return clk_gate_inverted(name, parent_name, reg, shift);
}

static inline struct clk *mxs_clk_mux(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parent_names, int num_parents)
{
	return clk_mux(name, reg, shift, width, parent_names, num_parents);
}

static inline struct clk *mxs_clk_fixed_factor(const char *name,
		const char *parent_name, unsigned int mult, unsigned int div)
{
	return clk_fixed_factor(name, parent_name, mult, div);
}

#endif /* __MXS_CLK_H */
