/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file
 * @brief Clocksource based on MIPS CP0 timer
 */

#include <init.h>
#include <of.h>
#include <linux/clk.h>
#include <clock.h>
#include <io.h>
#include <asm/mipsregs.h>

static uint64_t c0_hpt_read(void)
{
	return read_c0_count();
}

static struct clocksource cs = {
	.read	= c0_hpt_read,
	.mask	= CLOCKSOURCE_MASK(32),
};

static int clocksource_init(void)
{
	unsigned int mips_hpt_frequency;
	struct device_node *np;
	struct clk *clk;

	/* default rate: 100 MHz */
	mips_hpt_frequency = 100000000;

	if (IS_ENABLED(CONFIG_OFTREE)) {
		np = of_get_cpu_node(0, NULL);
		if (np) {
			clk = of_clk_get(np, 0);
			if (!IS_ERR(clk)) {
				mips_hpt_frequency = clk_get_rate(clk) / 2;
			}
		}
	}

	clocks_calc_mult_shift(&cs.mult, &cs.shift,
		mips_hpt_frequency, NSEC_PER_SEC, 10);

	return init_clock(&cs);
}
postcore_initcall(clocksource_init);
