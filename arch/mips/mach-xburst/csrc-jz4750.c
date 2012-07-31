/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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
 * @brief Clocksource based on JZ475x OS timer
 */

#include <init.h>
#include <clock.h>
#include <io.h>
#include <mach/jz4750d_regs.h>

#define JZ_TIMER_CLOCK 24000000

static uint64_t jz4750_cs_read(void)
{
	return (uint64_t)__raw_readl((void *)TCU_OSTCNT);
}

static struct clocksource jz4750_cs = {
	.read	= jz4750_cs_read,
	.mask   = CLOCKSOURCE_MASK(32),
};

static int clocksource_init(void)
{
	clocks_calc_mult_shift(&jz4750_cs.mult, &jz4750_cs.shift,
		JZ_TIMER_CLOCK, NSEC_PER_SEC, 10);

	init_clock(&jz4750_cs);

	__raw_writel(TCU_OSTCSR_PRESCALE1 | TCU_OSTCSR_EXT_EN,
		(void *)TCU_OSTCSR);
	__raw_writel(0, (void *)TCU_OSTCNT);
	__raw_writel(0xffffffff, (void *)TCU_OSTDR);

	/* enable timer clock */
	__raw_writel(TCU_TSCR_OSTSC, (void *)TCU_TSCR);
	/* start counting up */
	__raw_writel(TCU_TESR_OSTST, (void *)TCU_TESR);

	return 0;
}
core_initcall(clocksource_init);
