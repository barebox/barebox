/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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
#include <notifier.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <io.h>

#define TIMROTCTRL 0x00
#define TIMCTRL1 0x40
#define TIMCTRL1_SET 0x44
#define TIMCTRL1_CLR 0x48
#define TIMCTRL1_TOG 0x4c
# define TIMCTRL_RELOAD (1 << 6)
# define TIMCTRL_UPDATE (1 << 7)
# define TIMCTRL_PRESCALE(x) ((x & 0x3) << 4)
# define TIMCTRL_SELECT(x) (x & 0xf)
#define TIMCOUNT1 0x50

static const unsigned long timer_base = IMX_TIM1_BASE;

#define CLOCK_TICK_RATE (32000)

static uint64_t imx23_clocksource_read(void)
{
	/* only the upper bits are the valid */
	return ~(readl(timer_base + TIMCOUNT1) >> 16);
}

static struct clocksource cs = {
	.read	= imx23_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(16),
	.shift	= 10,
};

static int imx23_clocksource_clock_change(struct notifier_block *nb, unsigned long event, void *data)
{
	cs.mult = clocksource_hz2mult(CLOCK_TICK_RATE/*imx_get_xclk()*/, cs.shift);
	return 0;
}

static struct notifier_block imx23_clock_notifier = {
	.notifier_call = imx23_clocksource_clock_change,
};

static int clocksource_init(void)
{
	/* enable the whole timer block */
	writel(0x3e000000, timer_base + TIMROTCTRL);
	/* setup general purpose timer 1 */
	writel(0x00000000, timer_base + TIMCTRL1);
	writel(TIMCTRL_UPDATE, timer_base + TIMCTRL1);
	writel(0x0000ffff, timer_base + TIMCOUNT1);

	writel(TIMCTRL_UPDATE | TIMCTRL_RELOAD | TIMCTRL_PRESCALE(0) | TIMCTRL_SELECT(8), timer_base + TIMCTRL1);
	cs.mult = clocksource_hz2mult(CLOCK_TICK_RATE/*imx_get_xclk()*/, cs.shift);
	init_clock(&cs);

	clock_register_client(&imx23_clock_notifier);
	return 0;
}

core_initcall(clocksource_init);
