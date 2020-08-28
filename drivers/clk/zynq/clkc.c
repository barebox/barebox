// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013 Josh Cartwright <joshc@eso.teric.us>
 * Copyright (c) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * Based on drivers/clk-zynq.c from Linux.
 *
 * Copyright (c) 2012 National Instruments
 *
 * Josh Cartwright <josh.cartwright@ni.com>
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/zynq7000-regs.h>
#include <malloc.h>

enum zynq_clk {
	armpll, ddrpll, iopll,
	cpu_6or4x, cpu_3or2x, cpu_2x, cpu_1x,
	ddr2x, ddr3x, dci,
	lqspi, smc, pcap, gem0, gem1, fclk0, fclk1, fclk2, fclk3, can0, can1,
	sdio0, sdio1, uart0, uart1, spi0, spi1, dma,
	usb0_aper, usb1_aper, gem0_aper, gem1_aper,
	sdio0_aper, sdio1_aper, spi0_aper, spi1_aper, can0_aper, can1_aper,
	i2c0_aper, i2c1_aper, uart0_aper, uart1_aper, gpio_aper, lqspi_aper,
	smc_aper, swdt, dbg_trc, dbg_apb, clk_max
};

enum zynq_pll_type {
	ZYNQ_PLL_ARM,
	ZYNQ_PLL_DDR,
	ZYNQ_PLL_IO,
};

#define PLL_STATUS_ARM_PLL_LOCK		(1 << 0)
#define PLL_STATUS_DDR_PLL_LOCK		(1 << 1)
#define PLL_STATUS_IO_PLL_LOCK		(1 << 2)
#define PLL_STATUS_ARM_PLL_STABLE	(1 << 0)
#define PLL_STATUS_DDR_PLL_STABLE	(1 << 1)
#define PLL_STATUS_IO_PLL_STABLE	(1 << 2)
#define PLL_CTRL_BYPASS_FORCE		(1 << 4)
#define PLL_CTRL_PWRDOWN		(1 << 1)
#define PLL_CTRL_RESET			(1 << 0)

static struct clk *clks[clk_max];
static struct clk_onecell_data clk_data;

struct zynq_pll_clk {
	struct clk	clk;
	u32		pll_lock;
	void __iomem	*pll_ctrl;
};

#define to_zynq_pll_clk(c)	container_of(c, struct zynq_pll_clk, clk)

#define PLL_CTRL_FDIV(x)	(((x) >> 12) & 0x7F)

static unsigned long zynq_pll_recalc_rate(struct clk *clk,
					  unsigned long parent_rate)
{
	struct zynq_pll_clk *pll = to_zynq_pll_clk(clk);
	return parent_rate * PLL_CTRL_FDIV(readl(pll->pll_ctrl));
}

static int zynq_pll_enable(struct clk *clk)
{
	struct zynq_pll_clk *pll = to_zynq_pll_clk(clk);
	u32 val;
	int timeout = 10000;

	val = readl(pll->pll_ctrl);
	val &= ~(PLL_CTRL_BYPASS_FORCE | PLL_CTRL_PWRDOWN | PLL_CTRL_RESET);
	writel(val, pll->pll_ctrl);

	while (timeout--) {
		if (readl(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_PLL_STATUS) & pll->pll_lock)
			break;
	}

	if (!timeout)
		return -ETIMEDOUT;

	return 0;
}

static int zynq_pll_is_enabled(struct clk *clk)
{
	struct zynq_pll_clk *pll = to_zynq_pll_clk(clk);
	u32 val = readl(pll->pll_ctrl);

	return !(val & (PLL_CTRL_PWRDOWN | PLL_CTRL_RESET));
}

static struct clk_ops zynq_pll_clk_ops = {
	.recalc_rate = zynq_pll_recalc_rate,
	.enable = zynq_pll_enable,
	.is_enabled = zynq_pll_is_enabled,
};

static inline struct clk *zynq_pll_clk(enum zynq_pll_type type,
				       const char *name,
				       void __iomem *pll_ctrl)
{
	static const char *pll_parent = "ps_clk";
	struct zynq_pll_clk *pll;
	int ret;

	pll = xzalloc(sizeof(*pll));
	pll->pll_ctrl		= pll_ctrl;
	pll->clk.ops		= &zynq_pll_clk_ops;
	pll->clk.name		= name;
	pll->clk.parent_names	= &pll_parent;
	pll->clk.num_parents	= 1;

	switch(type) {
	case ZYNQ_PLL_ARM:
		pll->pll_lock = PLL_STATUS_ARM_PLL_LOCK;
		break;
	case ZYNQ_PLL_DDR:
		pll->pll_lock = PLL_STATUS_DDR_PLL_LOCK;
		break;
	case ZYNQ_PLL_IO:
		pll->pll_lock = PLL_STATUS_IO_PLL_LOCK;
		break;
	}

	ret = clk_register(&pll->clk);
	if (ret) {
		free(pll);
		return ERR_PTR(ret);
	}

	return &pll->clk;
}

struct zynq_periph_clk {
	struct clk	clk;
	void __iomem	*clk_ctrl;
};

#define to_zynq_periph_clk(c)  container_of(c, struct zynq_periph_clk, c)

static const u8 periph_clk_parent_map[] = {
	0, 0, 1, 2
};
#define PERIPH_CLK_CTRL_SRC(x) (periph_clk_parent_map[((x) & 0x30) >> 4])
#define PERIPH_CLK_CTRL_DIV(x) (((x) & 0x3F00) >> 8)

static unsigned long zynq_periph_recalc_rate(struct clk *clk,
					     unsigned long parent_rate)
{
	struct zynq_periph_clk *periph = to_zynq_periph_clk(clk);
	return parent_rate / PERIPH_CLK_CTRL_DIV(readl(periph->clk_ctrl));
}

static int zynq_periph_get_parent(struct clk *clk)
{
	struct zynq_periph_clk *periph = to_zynq_periph_clk(clk);
	return PERIPH_CLK_CTRL_SRC(readl(periph->clk_ctrl));
}

static const struct clk_ops zynq_periph_clk_ops = {
	.recalc_rate	= zynq_periph_recalc_rate,
	.get_parent	= zynq_periph_get_parent,
};

static struct clk *zynq_periph_clk(const char *name, void __iomem *clk_ctrl)
{
	static const char *peripheral_parents[] = {
		"io_pll",
		"arm_pll",
		"ddr_pll",
	};
	struct zynq_periph_clk *periph;
	int ret;

	periph = xzalloc(sizeof(*periph));

	periph->clk_ctrl	= clk_ctrl;
	periph->clk.name	= name;
	periph->clk.ops		= &zynq_periph_clk_ops;
	periph->clk.parent_names = peripheral_parents;
	periph->clk.num_parents	= ARRAY_SIZE(peripheral_parents);

	ret = clk_register(&periph->clk);
	if (ret) {
		free(periph);
		return ERR_PTR(ret);
	}

	return &periph->clk;
}

/* CPU Clock domain is modelled as a mux with 4 children subclks, whose
 * derivative rates depend on CLK_621_TRUE
 */

struct zynq_cpu_clk {
	struct clk	clk;
	void __iomem	*clk_ctrl;
};

#define to_zynq_cpu_clk(c)	container_of(c, struct zynq_cpu_clk, c)

static const u8 zynq_cpu_clk_parent_map[] = {
	1, 1, 2, 0
};
#define CPU_CLK_SRCSEL(x)	(zynq_cpu_clk_parent_map[(((x) & 0x30) >> 4)])
#define CPU_CLK_CTRL_DIV(x)	(((x) & 0x3F00) >> 8)

static unsigned long zynq_cpu_clk_recalc_rate(struct clk *clk,
					      unsigned long parent_rate)
{
	struct zynq_cpu_clk *cpuclk = to_zynq_cpu_clk(clk);
	return parent_rate / CPU_CLK_CTRL_DIV(readl(cpuclk->clk_ctrl));
}

static int zynq_cpu_clk_get_parent(struct clk *clk)
{
	struct zynq_cpu_clk *cpuclk = to_zynq_cpu_clk(clk);
	return CPU_CLK_SRCSEL(readl(cpuclk->clk_ctrl));
}

static const struct clk_ops zynq_cpu_clk_ops = {
	.get_parent	= zynq_cpu_clk_get_parent,
	.recalc_rate	= zynq_cpu_clk_recalc_rate,
};

static struct clk *zynq_cpu_clk(const char *name, void __iomem *clk_ctrl)
{
	static const char *cpu_parents[] = {
		"io_pll",
		"arm_pll",
		"ddr_pll",
	};
	struct zynq_cpu_clk *cpu;
	int ret;

	cpu = xzalloc(sizeof(*cpu));

	cpu->clk_ctrl		= clk_ctrl;
	cpu->clk.ops		= &zynq_cpu_clk_ops;
	cpu->clk.name		= name;
	cpu->clk.parent_names	= cpu_parents;
	cpu->clk.num_parents	= ARRAY_SIZE(cpu_parents);

	ret = clk_register(&cpu->clk);
	if (ret) {
		free(cpu);
		return ERR_PTR(ret);
	}

	return &cpu->clk;
}

enum zynq_cpu_subclk_which {
	CPU_SUBCLK_6X4X,
	CPU_SUBCLK_3X2X,
	CPU_SUBCLK_2X,
	CPU_SUBCLK_1X,
};

struct zynq_cpu_subclk {
	struct clk	clk;
	void __iomem	*clk_ctrl;
	void __iomem	*clk_621;
	enum zynq_cpu_subclk_which which;
};

#define CLK_621_TRUE(x)		((x) & 1)

#define to_zynq_cpu_subclk(c)	container_of(c, struct zynq_cpu_subclk, c);

static unsigned long zynq_cpu_subclk_recalc_rate(struct clk *clk,
						 unsigned long parent_rate)
{
	unsigned long uninitialized_var(rate);
	struct zynq_cpu_subclk *subclk;
	bool is_621;

	subclk = to_zynq_cpu_subclk(clk);
	is_621 = CLK_621_TRUE(readl(subclk->clk_621));

	switch (subclk->which) {
	case CPU_SUBCLK_6X4X:
		rate = parent_rate;
		break;
	case CPU_SUBCLK_3X2X:
		rate = parent_rate / 2;
		break;
	case CPU_SUBCLK_2X:
		rate = parent_rate / (is_621 ? 3 : 2);
		break;
	case CPU_SUBCLK_1X:
		rate = parent_rate / (is_621 ? 6 : 4);
		break;
	};

	return rate;
}

static int zynq_cpu_subclk_enable(struct clk *clk)
{
	struct zynq_cpu_subclk *subclk;
	u32 tmp;

	subclk = to_zynq_cpu_subclk(clk);

	tmp = readl(subclk->clk_ctrl);
	tmp |= 1 << (24 + subclk->which);
	writel(tmp, subclk->clk_ctrl);

	return 0;
}

static void zynq_cpu_subclk_disable(struct clk *clk)
{
	struct zynq_cpu_subclk *subclk;
	u32 tmp;

	subclk = to_zynq_cpu_subclk(clk);

	tmp = readl(subclk->clk_ctrl);
	tmp &= ~(1 << (24 + subclk->which));
	writel(tmp, subclk->clk_ctrl);
}

static const struct clk_ops zynq_cpu_subclk_ops = {
	.enable		= zynq_cpu_subclk_enable,
	.disable	= zynq_cpu_subclk_disable,
	.recalc_rate	= zynq_cpu_subclk_recalc_rate,
};

static struct clk *zynq_cpu_subclk(const char *name,
				   enum zynq_cpu_subclk_which which,
				   void __iomem *clk_ctrl,
				   void __iomem *clk_621)
{
	static const char *subclk_parent = "cpu_clk";
	struct zynq_cpu_subclk *subclk;
	int ret;

	subclk = xzalloc(sizeof(*subclk));

	subclk->clk_ctrl	= clk_ctrl;
	subclk->clk_621		= clk_621;
	subclk->which		= which;
	subclk->clk.name	= name;
	subclk->clk.ops		= &zynq_cpu_subclk_ops;

	subclk->clk.parent_names = &subclk_parent;
	subclk->clk.num_parents	= 1;

	ret = clk_register(&subclk->clk);
	if (ret) {
		free(subclk);
		return ERR_PTR(ret);
	}

	return &subclk->clk;
}

static int zynq_clock_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *clk_base;
	unsigned long ps_clk_rate = 33333330;
	resource_size_t slcr_offset = 0;

	iores = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	/*
	 * The Zynq 7000 DT describes the SLCR child devices with a reg offset
	 * in the SCLR region. So we can't directly map the address we get from
	 * the DT, but need to add the SCLR base offset.
	 */
	if (dev->device_node) {
		struct resource *parent_res;

		parent_res = dev_get_resource(dev->parent, IORESOURCE_MEM, 0);
		if (IS_ERR(parent_res))
			return PTR_ERR(parent_res);

		slcr_offset = parent_res->start;
	}

	iores = request_iomem_region(dev_name(dev), iores->start + slcr_offset,
				     iores->end + slcr_offset);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	clk_base = IOMEM(iores->start);

	clk_fixed("ps_clk", ps_clk_rate);

	clks[armpll] = zynq_pll_clk(ZYNQ_PLL_ARM, "arm_pll", clk_base + 0x0);
	clks[ddrpll] = zynq_pll_clk(ZYNQ_PLL_DDR, "ddr_pll", clk_base + 0x4);
	clks[iopll] = zynq_pll_clk(ZYNQ_PLL_IO,  "io_pll", clk_base + 0x8);

	zynq_periph_clk("lqspi_clk", clk_base + 0x4c);
	clks[lqspi] = clk_gate("qspi0", "lqspi_clk", clk_base + 0x4c, 0, 0, 0);

	zynq_periph_clk("sdio_clk", clk_base + 0x50);
	clks[sdio0] = clk_gate("sdio0", "sdio_clk", clk_base + 0x50, 0, 0, 0);
	clks[sdio1] = clk_gate("sdio1", "sdio_clk", clk_base + 0x50, 1, 0, 0);

	zynq_periph_clk("uart_clk", clk_base + 0x54);
	clks[uart0] = clk_gate("uart0", "uart_clk", clk_base + 0x54, 0, 0, 0);
	clks[uart1] = clk_gate("uart1", "uart_clk", clk_base + 0x54, 1, 0, 0);

	zynq_periph_clk("spi_clk", clk_base + 0x58);
	clks[spi0] = clk_gate("spi0", "spi_clk", clk_base + 0x58, 0, 0, 0);
	clks[spi1] = clk_gate("spi1", "spi_clk", clk_base + 0x58, 1, 0, 0);

	clks[gem0] = clk_gate("gem0", "io_pll", clk_base + 0x40, 0, 0, 0);
	clks[gem1] = clk_gate("gem1", "io_pll", clk_base + 0x44, 1, 0, 0);

	zynq_cpu_clk("cpu_clk", clk_base + 0x20);

	clks[cpu_6or4x] = zynq_cpu_subclk("cpu_6x4x", CPU_SUBCLK_6X4X,
					clk_base + 0x20, clk_base + 0xC4);
	clks[cpu_3or2x] = zynq_cpu_subclk("cpu_3x2x", CPU_SUBCLK_3X2X,
					clk_base + 0x20, clk_base + 0xC4);
	clks[cpu_2x] = zynq_cpu_subclk("cpu_2x", CPU_SUBCLK_2X,
					clk_base + 0x20, clk_base + 0xC4);
	clks[cpu_1x] = zynq_cpu_subclk("cpu_1x", CPU_SUBCLK_1X,
					clk_base + 0x20, clk_base + 0xC4);

	clks[dma] = clk_gate("dma", "cpu_2x", clk_base + 0x2C, 0, 0, 0);
	clks[usb0_aper] = clk_gate("usb0_aper", "cpu_1x",
				   clk_base + 0x2C, 2, 0, 0);
	clks[usb1_aper] = clk_gate("usb1_aper", "cpu_1x",
				   clk_base + 0x2C, 3, 0, 0);
	clks[gem0_aper] = clk_gate("gem0_aper", "cpu_1x",
				   clk_base + 0x2C, 6, 0, 0);
	clks[gem1_aper] = clk_gate("gem1_aper", "cpu_1x",
				   clk_base + 0x2C, 7, 0, 0);
	clks[sdio0_aper] = clk_gate("sdio0_aper", "cpu_1x",
				    clk_base + 0x2C, 10, 0, 0);
	clks[sdio1_aper] = clk_gate("sdio1_aper", "cpu_1x",
				    clk_base + 0x2C, 11, 0, 0);
	clks[spi0_aper] = clk_gate("spi0_aper", "cpu_1x",
				   clk_base + 0x2C, 14, 0, 0);
	clks[spi1_aper] = clk_gate("spi1_aper", "cpu_1x",
				   clk_base + 0x2C, 15, 0, 0);
	clks[can0_aper] = clk_gate("can0_aper", "cpu_1x",
				   clk_base + 0x2C, 16, 0, 0);
	clks[can1_aper] = clk_gate("can1_aper", "cpu_1x",
				   clk_base + 0x2C, 17, 0, 0);
	clks[i2c0_aper] = clk_gate("i2c0_aper", "cpu_1x",
				   clk_base + 0x2C, 18, 0, 0);
	clks[i2c1_aper] = clk_gate("i2c1_aper", "cpu_1x",
				   clk_base + 0x2C, 19, 0, 0);
	clks[uart0_aper] = clk_gate("uart0_aper", "cpu_1x",
				    clk_base + 0x2C, 20, 0, 0);
	clks[uart1_aper] = clk_gate("uart1_aper", "cpu_1x",
				    clk_base + 0x2C, 21, 0, 0);
	clks[gpio_aper] = clk_gate("gpio_aper", "cpu_1x",
				   clk_base + 0x2C, 22, 0, 0);
	clks[lqspi_aper] = clk_gate("lqspi_aper", "cpu_1x",
				    clk_base + 0x2C, 23, 0, 0);
	clks[smc_aper] = clk_gate("smc_aper", "cpu_1x",
				  clk_base + 0x2C, 24, 0, 0);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get,
			    &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id zynq_clock_dt_ids[] = {
	{
		.compatible = "xlnx,ps7-clkc",
	}, {
		/* sentinel */
	}
};

static struct driver_d zynq_clock_driver = {
	.probe  = zynq_clock_probe,
	.name   = "zynq-clock",
	.of_compatible = DRV_OF_COMPAT(zynq_clock_dt_ids),
};

postcore_platform_driver(zynq_clock_driver);
