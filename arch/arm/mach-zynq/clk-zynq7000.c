/*
 * Copyright (c) 2013 Josh Cartwright <joshc@eso.teric.us>
 * Copyright (c) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * Based on drivers/clk-zynq.c from Linux.
 *
 * Copyright (c) 2012 National Instruments
 *
 * Josh Cartwright <josh.cartwright@ni.com>
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
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/zynq7000-regs.h>
#include <malloc.h>

enum zynq_clks {
	dummy, ps_clk, arm_pll, ddr_pll, io_pll, uart_clk, uart0, uart1,
	cpu_clk, cpu_6x4x, cpu_3x2x, cpu_2x, cpu_1x,
	gem_clk, gem0, gem1, clks_max
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

static struct clk *clks[clks_max];

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
	val &= ~PLL_CTRL_BYPASS_FORCE;
	writel(val, pll->pll_ctrl);

	while (timeout--) {
		if (readl(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_PLL_STATUS) & pll->pll_lock)
			break;
	}

	if (!timeout)
		return -ETIMEDOUT;

	return 0;
}

static struct clk_ops zynq_pll_clk_ops = {
	.recalc_rate = zynq_pll_recalc_rate,
	.enable = zynq_pll_enable,
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
	void __iomem *slcr_base;
	unsigned long ps_clk_rate = 33333330;

	slcr_base = dev_request_mem_region(dev, 0);
	if (!slcr_base)
		return -EBUSY;

	clks[ps_clk]  = clk_fixed("ps_clk", ps_clk_rate);

	clks[arm_pll] = zynq_pll_clk(ZYNQ_PLL_ARM, "arm_pll", slcr_base + 0x100);
	clks[ddr_pll] = zynq_pll_clk(ZYNQ_PLL_DDR, "ddr_pll", slcr_base + 0x104);
	clks[io_pll]  = zynq_pll_clk(ZYNQ_PLL_IO,  "io_pll", slcr_base + 0x108);

	clks[uart_clk] = zynq_periph_clk("uart_clk", slcr_base + 0x154);

	clks[uart0] = clk_gate("uart0", "uart_clk", slcr_base + 0x154, 0);
	clks[uart1] = clk_gate("uart1", "uart_clk", slcr_base + 0x154, 1);

	clks[gem0] = clk_gate("gem0", "io_pll", slcr_base + 0x140, 0);
	clks[gem1] = clk_gate("gem1", "io_pll", slcr_base + 0x144, 1);

	clks[cpu_clk] = zynq_cpu_clk("cpu_clk", slcr_base + 0x120);

	clks[cpu_6x4x] = zynq_cpu_subclk("cpu_6x4x", CPU_SUBCLK_6X4X,
					slcr_base + 0x120, slcr_base + 0x1C4);
	clks[cpu_3x2x] = zynq_cpu_subclk("cpu_3x2x", CPU_SUBCLK_3X2X,
					slcr_base + 0x120, slcr_base + 0x1C4);
	clks[cpu_2x] = zynq_cpu_subclk("cpu_2x", CPU_SUBCLK_2X,
					slcr_base + 0x120, slcr_base + 0x1C4);
	clks[cpu_1x] = zynq_cpu_subclk("cpu_1x", CPU_SUBCLK_1X,
					slcr_base + 0x120, slcr_base + 0x1C4);

	clk_register_clkdev(clks[cpu_3x2x], NULL, "arm_smp_twd");
	clk_register_clkdev(clks[uart0], NULL, "zynq_serial0");
	clk_register_clkdev(clks[uart1], NULL, "zynq_serial1");
	clk_register_clkdev(clks[gem0], NULL, "macb0");
	clk_register_clkdev(clks[gem1], NULL, "macb1");

	clkdev_add_physbase(clks[cpu_3x2x], CORTEXA9_SCU_TIMER_BASE_ADDR, NULL);
	clkdev_add_physbase(clks[uart1], ZYNQ_UART1_BASE_ADDR, NULL);

	return 0;
}

static __maybe_unused struct of_device_id zynq_clock_dt_ids[] = {
	{
		.compatible = "xlnx,zynq-clock",
	}, {
		/* sentinel */
	}
};

static struct driver_d zynq_clock_driver = {
	.probe  = zynq_clock_probe,
	.name   = "zynq-clock",
	.of_compatible = DRV_OF_COMPAT(zynq_clock_dt_ids),
};

static int zynq_clock_init(void)
{
	return platform_driver_register(&zynq_clock_driver);
}
postcore_initcall(zynq_clock_init);
