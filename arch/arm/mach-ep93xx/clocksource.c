/*
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include <io.h>
#include <mach/ep93xx-regs.h>

#define TIMER_CLKSEL		(1 << 3)
#define TIMER_MODE		(1 << 6)
#define TIMER_ENABLE		(1 << 7)

#define TIMER_FREQ		508469

static uint64_t ep93xx_clocksource_read(void)
{
	struct timer_regs *timer = (struct timer_regs *)TIMER_BASE;

	return 0xffffffff - readl(&timer->timer3.value);
}

static struct clocksource cs = {
	.read	= ep93xx_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int clocksource_init(void)
{
	struct timer_regs *timer = (struct timer_regs *)TIMER_BASE;

	/* use timer 3 with 508KHz and free running */
	writel(TIMER_CLKSEL,
	       &timer->timer3.control);

	/* load timer 3 with max value */
	writel(0xffffffff, &timer->timer3.load);

	/* enable timer 3 with 508KHz and periodic mode */
	writel(TIMER_ENABLE | TIMER_MODE | TIMER_CLKSEL,
	       &timer->timer3.control);

	cs.mult = clocksource_hz2mult(TIMER_FREQ, cs.shift);

	init_clock(&cs);

	return 0;
}

core_initcall(clocksource_init);

/*
 * Reset the cpu
 */
void __noreturn reset_cpu(unsigned long addr)
{
	struct syscon_regs *syscon = (struct syscon_regs *)SYSCON_BASE;
	uint32_t value;

	/* Unlock DeviceCfg and set SWRST */
	writel(0xAA, &syscon->sysswlock);
	value = readl(&syscon->devicecfg);
	value |= SYSCON_DEVICECFG_SWRST;
	writel(value, &syscon->devicecfg);

	/* Unlock DeviceCfg and clear SWRST */
	writel(0xAA, &syscon->sysswlock);
	value = readl(&syscon->devicecfg);
	value &= ~SYSCON_DEVICECFG_SWRST;
	writel(value, &syscon->devicecfg);

	/* Dying... */
	while (1)
		; /* noop */
}
EXPORT_SYMBOL(reset_cpu);
