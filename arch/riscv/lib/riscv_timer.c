// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file
 * @brief Clocksource based on RISC-V cycle CSR timer
 */

#include <init.h>
#include <of.h>
#include <linux/clk.h>
#include <clock.h>

static uint64_t rdcycle_read(void)
{
	register unsigned long __v;

	__asm__ __volatile__ ("rdcycle %0" : "=r" (__v));

	return __v;
}

static struct clocksource rdcycle_cs = {
	.read	= rdcycle_read,
	.mask	= CLOCKSOURCE_MASK(32),
};

static int rdcycle_cs_init(void)
{
	unsigned int cycle_frequency;

	/* default rate: 100 MHz */
	cycle_frequency = 100000000;

	if (IS_ENABLED(CONFIG_OFTREE)) {
		struct device_node *np;
		struct clk *clk;

		np = of_get_cpu_node(0, NULL);
		if (np) {
			clk = of_clk_get(np, 0);
			if (!IS_ERR(clk)) {
				cycle_frequency = clk_get_rate(clk);
			}
		}
	}

	clocks_calc_mult_shift(&rdcycle_cs.mult, &rdcycle_cs.shift,
		cycle_frequency, NSEC_PER_SEC, 10);

	return init_clock(&rdcycle_cs);
}
postcore_initcall(rdcycle_cs_init);
