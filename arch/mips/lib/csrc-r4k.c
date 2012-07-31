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
#include <clock.h>
#include <io.h>
#include <asm/mipsregs.h>

static uint64_t c0_hpt_read(void)
{
	return read_c0_count();
}

static struct clocksource cs = {
	.read	= c0_hpt_read,
	.mask	= 0xffffffff,
};

static int clocksource_init(void)
{
	cs.mult = clocksource_hz2mult(100000000, cs.shift);
	init_clock(&cs);

	return 0;
}
core_initcall(clocksource_init);
