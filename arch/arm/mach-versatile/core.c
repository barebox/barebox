/*
 * Copyright (C) 2010 B Labs Ltd,
 * http://l4dev.org
 * Author: Alexey Zaytsev <alexey.zaytsev@gmail.com>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Copyright (C) 1999 - 2003 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <debug_ll.h>
#include <restart.h>
#include <linux/sizes.h>

#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/amba/bus.h>

#include <io.h>
#include <asm/hardware/arm_timer.h>
#include <asm/armlinux.h>

#include <mach/versatile/platform.h>


/* 1Mhz / 256 */
#define TIMER_FREQ (1000000/256)

#define TIMER0_BASE (VERSATILE_TIMER0_1_BASE)
#define TIMER1_BASE ((VERSATILE_TIMER0_1_BASE) + 0x20)
#define TIMER2_BASE (VERSATILE_TIMER2_3_BASE)
#define TIMER3_BASE ((VERSATILE_TIMER2_3_BASE) + 0x20)

static uint64_t vpb_clocksource_read(void)
{
	return ~readl(TIMER0_BASE + TIMER_VALUE);
}

static struct clocksource vpb_cs = {
	.read = vpb_clocksource_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 10,
	.priority = 80,
};

/* From Linux v2.6.35
 * arch/arm/mach-versatile/core.c */
static void versatile_timer_init (void)
{
	u32 val;

	/*
	 * set clock frequency:
	 *      VERSATILE_REFCLK is 32KHz
	 *      VERSATILE_TIMCLK is 1MHz
	 */

	val = readl(VERSATILE_SCTL_BASE);
	val |= (VERSATILE_TIMCLK << VERSATILE_TIMER1_EnSel);
	writel(val, VERSATILE_SCTL_BASE);

	/*
	 * Disable all timers, just to be sure.
	 */

	writel(0, TIMER0_BASE + TIMER_CTRL);
	writel(0, TIMER1_BASE + TIMER_CTRL);
	writel(0, TIMER2_BASE + TIMER_CTRL);
	writel(0, TIMER3_BASE + TIMER_CTRL);

	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_DIV256,
					TIMER0_BASE + TIMER_CTRL);
}

static int vpb_clocksource_init(void)
{
	versatile_timer_init();
	vpb_cs.mult = clocksource_hz2mult(TIMER_FREQ, vpb_cs.shift);

	return init_clock(&vpb_cs);
}

static void __noreturn versatile_reset_soc(struct restart_handler *rst)
{
	u32 val;

	val = __raw_readl(VERSATILE_SYS_RESETCTL) & ~0x7;
	val |= 0x105;

	__raw_writel(0xa05f, VERSATILE_SYS_LOCK);
	__raw_writel(val, VERSATILE_SYS_RESETCTL);
	__raw_writel(0, VERSATILE_SYS_LOCK);

	hang();
}

static int versatile_init(void)
{
	if (!of_machine_is_compatible("arm,versatile-pb") &&
	    !of_machine_is_compatible("arm,versatile-ab"))
		return 0;

	vpb_clocksource_init();
	restart_handler_register_fn("soc", versatile_reset_soc);
	return 0;
}
core_initcall(versatile_init);
