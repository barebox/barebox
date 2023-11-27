#ifndef __SOC_IMX_CLK_FRACN_GPGPPLL_H
#define __SOC_IMX_CLK_FRACN_GPGPPLL_H

#include <linux/bitfield.h>
#include <linux/iopoll.h>

#define GPPLL_CTRL		0x0
#define HW_CTRL_SEL		BIT(16)
#define CLKMUX_BYPASS		BIT(2)
#define CLKMUX_EN		BIT(1)
#define POWERUP_MASK		BIT(0)

#define GPPLL_ANA_PRG		0x10
#define GPPLL_SPREAD_SPECTRUM	0x30

#define GPPLL_NUMERATOR		0x40
#define GPPLL_MFN_MASK		GENMASK(31, 2)

#define GPPLL_DENOMINATOR	0x50
#define GPPLL_MFD_MASK		GENMASK(29, 0)

#define GPPLL_DIV		0x60
#define GPPLL_MFI_MASK		GENMASK(24, 16)
#define GPPLL_RDIV_MASK		GENMASK(15, 13)
#define GPPLL_ODIV_MASK		GENMASK(7, 0)

#define GPPLL_DFS_CTRL(x)	(0x70 + (x) * 0x10)

#define GPPLL_STATUS		0xF0
#define GPPLL_LOCK_STATUS	BIT(0)

#define GPPLL_DFS_STATUS	0xF4

#define GPPLL_LOCK_TIMEOUT_US	200

#define CLK_FRACN_GPPLL_INTEGER BIT(0)
#define CLK_FRACN_GPPLL_FRACN   BIT(1)

/* NOTE: Rate table should be kept sorted in descending order. */
struct imx_fracn_gppll_rate_table {
	unsigned int rate;
	unsigned int mfi;
	unsigned int mfn;
	unsigned int mfd;
	unsigned int rdiv;
	unsigned int odiv;
};

struct imx_fracn_gppll_clk {
	const struct imx_fracn_gppll_rate_table *rate_table;
	int rate_count;
	int flags;
};

struct clk *imx_clk_fracn_gppll(const char *name, const char *parent_name, void __iomem *base,
				const struct imx_fracn_gppll_clk *pll_clk);
struct clk *imx_clk_fracn_gppll_integer(const char *name, const char *parent_name,
					void __iomem *base,
					const struct imx_fracn_gppll_clk *pll_clk);

extern struct imx_fracn_gppll_clk imx_fracn_gppll;
extern struct imx_fracn_gppll_clk imx_fracn_gppll_integer;

static inline int fracn_gppll_wait_lock(void __iomem *base)
{
	u32 val;

	return readl_poll_timeout(base + GPPLL_STATUS, val,
				  val & GPPLL_LOCK_STATUS, GPPLL_LOCK_TIMEOUT_US);
}

static inline const struct imx_fracn_gppll_rate_table *imx_get_gppll_settings(
	const struct imx_fracn_gppll_rate_table *rate_table, int n_table, unsigned long rate)
{
	int i;

	for (i = 0; i < n_table; i++)
		if (rate == rate_table[i].rate)
			return &rate_table[i];

	return NULL;
}

static inline int fracn_gppll_set_rate(void __iomem *base, unsigned int flags,
				       const struct imx_fracn_gppll_rate_table *table, int n_table,
				       unsigned long drate)
{
	const struct imx_fracn_gppll_rate_table *rate;
	u32 tmp, pll_div, ana_mfn;
	int ret;

	rate = imx_get_gppll_settings(table, n_table, drate);

	/* Hardware control select disable. PLL is control by register */
	tmp = readl_relaxed(base + GPPLL_CTRL);
	tmp &= ~HW_CTRL_SEL;
	writel_relaxed(tmp, base + GPPLL_CTRL);

	/* Disable output */
	tmp = readl_relaxed(base + GPPLL_CTRL);
	tmp &= ~CLKMUX_EN;
	writel_relaxed(tmp, base + GPPLL_CTRL);

	/* Power Down */
	tmp &= ~POWERUP_MASK;
	writel_relaxed(tmp, base + GPPLL_CTRL);

	/* Disable BYPASS */
	tmp &= ~CLKMUX_BYPASS;
	writel_relaxed(tmp, base + GPPLL_CTRL);

	pll_div = FIELD_PREP(GPPLL_RDIV_MASK, rate->rdiv) | rate->odiv |
		FIELD_PREP(GPPLL_MFI_MASK, rate->mfi);
	writel_relaxed(pll_div, base + GPPLL_DIV);
	if (flags & CLK_FRACN_GPPLL_FRACN) {
		writel_relaxed(rate->mfd, base + GPPLL_DENOMINATOR);
		writel_relaxed(FIELD_PREP(GPPLL_MFN_MASK, rate->mfn), base + GPPLL_NUMERATOR);
	}

	/* Wait for 5us according to fracn mode pll doc */
	udelay(5);

	/* Enable Powerup */
	tmp |= POWERUP_MASK;
	writel_relaxed(tmp, base + GPPLL_CTRL);

	/* Wait Lock */
	ret = fracn_gppll_wait_lock(base);
	if (ret)
		return ret;

	/* Enable output */
	tmp |= CLKMUX_EN;
	writel_relaxed(tmp, base + GPPLL_CTRL);

	ana_mfn = readl_relaxed(base + GPPLL_STATUS);
	ana_mfn = FIELD_GET(GPPLL_MFN_MASK, ana_mfn);

	WARN(ana_mfn != rate->mfn, "ana_mfn != rate->mfn\n");

	return 0;
}

#endif /* __SOC_IMX_CLK_FRACN_GPGPPLL_H */
