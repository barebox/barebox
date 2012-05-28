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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief Clocksource based on Tegra internal timer
 */

#include <common.h>
#include <clock.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <init.h>
#include <asm/io.h>
#include <mach/iomap.h>

static void __iomem *timer_reg_base = (void __iomem *) (TEGRA_TMR1_BASE);

#define timer_writel(value, reg) \
	__raw_writel(value, (u32)timer_reg_base + (reg))
#define timer_readl(reg) \
	__raw_readl((u32)timer_reg_base + (reg))

static uint64_t tegra_clocksource_read(void)
{
	return timer_readl(0x10);
}

static struct clocksource cs = {
	.read	= tegra_clocksource_read,
	.mask	= 0xffffffff,
};

/* FIXME: here we have no initialization. All initialization made by U-Boot */
static int clocksource_init(void)
{
	cs.mult = clocksource_hz2mult(1000000, cs.shift);
	init_clock(&cs);

	return 0;
}
core_initcall(clocksource_init);
