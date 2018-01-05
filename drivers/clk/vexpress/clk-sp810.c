/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2013 ARM Limited
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <of_address.h>
#include <linux/clk.h>
#include <linux/err.h>

/* sysctl registers offset */
#define SCCTRL			0x000
#define SCCTRL_TIMERENnSEL_SHIFT(n)	(15 + ((n) * 2))

struct clk_sp810;

struct clk_sp810_timerclken {
	struct clk hw;
	struct clk_sp810 *sp810;
	int channel;
};

static inline struct clk_sp810_timerclken *
to_clk_sp810_timerclken(struct clk *clk)
{
	return container_of(clk, struct clk_sp810_timerclken, hw);
}

struct clk_sp810 {
	struct device_node *node;
	void __iomem *base;
	struct clk_sp810_timerclken timerclken[4];
};

static int clk_sp810_timerclken_get_parent(struct clk *hw)
{
	return 1;
}

static int clk_sp810_timerclken_set_parent(struct clk *hw, u8 index)
{
	struct clk_sp810_timerclken *timerclken = to_clk_sp810_timerclken(hw);
	struct clk_sp810 *sp810 = timerclken->sp810;
	u32 val, shift = SCCTRL_TIMERENnSEL_SHIFT(timerclken->channel);

	if (WARN_ON(index > 1))
		return -EINVAL;

	if (index == 0)
		return -EINVAL;

	val = readl(sp810->base + SCCTRL);
	val &= ~(1 << shift);
	val |= index << shift;
	writel(val, sp810->base + SCCTRL);

	return 0;
}

static const struct clk_ops clk_sp810_timerclken_ops = {
	.get_parent = clk_sp810_timerclken_get_parent,
	.set_parent = clk_sp810_timerclken_set_parent,
};

static struct clk *clk_sp810_timerclken_of_get(struct of_phandle_args *clkspec,
		void *data)
{
	struct clk_sp810 *sp810 = data;

	if (WARN_ON(clkspec->args_count != 1 ||
		    clkspec->args[0] >=	ARRAY_SIZE(sp810->timerclken)))
		return NULL;

	return &sp810->timerclken[clkspec->args[0]].hw;
}

static void clk_sp810_of_setup(struct device_node *node)
{
	struct clk_sp810 *sp810 = xzalloc(sizeof(*sp810));
	const char *parent_names[2];
	int num = ARRAY_SIZE(parent_names);
	char name[12];
	static int instance;
	int i;

	if (!sp810)
		return;

	if (of_clk_parent_fill(node, parent_names, num) != num) {
		pr_warn("Failed to obtain parent clocks for SP810!\n");
		kfree(sp810);
		return;
	}

	sp810->node = node;
	sp810->base = of_iomap(node, 0);

	for (i = 0; i < ARRAY_SIZE(sp810->timerclken); i++) {
		snprintf(name, sizeof(name), "sp810_%d_%d", instance, i);

		sp810->timerclken[i].sp810 = sp810;
		sp810->timerclken[i].channel = i;
		sp810->timerclken[i].hw.name = strdup(name);
		sp810->timerclken[i].hw.parent_names = parent_names;
		sp810->timerclken[i].hw.num_parents = num;
		sp810->timerclken[i].hw.ops = &clk_sp810_timerclken_ops;

		/*
		 * Always set parent to 1MHz clock to match QEMU emulation
		 * and satisfy requirements on real HW.
		 */
		clk_sp810_timerclken_set_parent(&sp810->timerclken[i].hw, 1);

		clk_register(&sp810->timerclken[i].hw);
	}

	of_clk_add_provider(node, clk_sp810_timerclken_of_get, sp810);
	instance++;
}
CLK_OF_DECLARE(sp810, "arm,sp810", clk_sp810_of_setup);
