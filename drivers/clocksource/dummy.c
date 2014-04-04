/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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

#include <common.h>
#include <init.h>
#include <clock.h>

static uint64_t dummy_counter;

static uint64_t dummy_cs_read(void)
{
	static int first;

	if (!first) {
		pr_warn("Warning: Using dummy clocksource\n");
		first = 1;
	}

	dummy_counter += CONFIG_CLOCKSOURCE_DUMMY_RATE;

	return dummy_counter;
}

static struct clocksource dummy_cs = {
	.read	= dummy_cs_read,
	.mask   = CLOCKSOURCE_MASK(32),
};

static int clocksource_init(void)
{
	dummy_counter = 0;

	clocks_calc_mult_shift(&dummy_cs.mult, &dummy_cs.shift,
		100000000, NSEC_PER_SEC, 10);

	pr_debug("clocksource_init: mult=%08x, shift=%08x\n",
			dummy_cs.mult, dummy_cs.shift);
	init_clock(&dummy_cs);

	return 0;
}
pure_initcall(clocksource_init);
