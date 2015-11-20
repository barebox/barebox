/*
 * clock.h - implementation of the PXA clock functions
 *
 * Copyright (C) 2014 by Robert Jarzmik <robert.jarzmik@free.fr>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <mach/clock.h>
#include <mach/pxa-regs.h>

/* Crystal clock: 13MHz */
#define BASE_CLK	13000000

unsigned long pxa_get_uartclk(void)
{
	return 14857000;
}

unsigned long pxa_get_pwmclk(void)
{
	return BASE_CLK;
}

static int pxa3xx_clock_init(void)
{
	unsigned long nand_rate = (cpu_is_pxa320()) ? 104000000 : 156000000;
	struct clk *clk;
	int ret;

	clk = clk_fixed("nand", nand_rate);
	ret = clk_register_clkdev(clk, NULL, "nand");
	if (ret)
		return ret;

	return 0;
}
postcore_initcall(pxa3xx_clock_init);
