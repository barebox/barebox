/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix <kernel@pengutronix.de>
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
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <notifier.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <io.h>

#define TIMROTCTRL 0x00
# define TIMROTCTRL_SFTRST
# define TIMROTCTRL_CLKGATE
# define TIMROTCTRL_DIVIDER(x) ((x & 0x3f) << 16)

#define TIMCTRL1 0x60
#define TIMCTRL1_SET 0x64
#define TIMCTRL1_CLR 0x68
#define TIMCTRL1_TOG 0x6c
# define TIMCTRL_RELOAD (1 << 6)
# define TIMCTRL_UPDATE (1 << 7)
# define TIMCTRL_PRESCALE(x) ((x & 0x3) << 4)
# define TIMCTRL_SELECT(x) (x & 0xf)
#define TIMCOUNT1 0x70
#define TIMFIX1 0x80

static const void __iomem * timer_base = (void *)IMX_TIM1_BASE;

/* we are using the 32 kHz reference */
#define CLOCK_TICK_RATE 32000

static uint64_t imx28_clocksource_read(void)
{
	return ~(readl(timer_base + TIMCOUNT1));
}

static struct clocksource imx28_cs = {
	.read	= imx28_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 17,
};

static int imx28_clocksource_init(void)
{
	/* enable the whole timer block */
	writel(0x00000000, timer_base + TIMROTCTRL);
	/* setup start value of the general purpose timer */
	writel(0x00000000, timer_base + TIMCTRL1);
	writel(TIMCTRL_UPDATE, timer_base + TIMCTRL1);
	/* setup the reload value of the general purpose timer */
	writel(0xffffffff, timer_base + TIMFIX1);

	writel(TIMCTRL_UPDATE | TIMCTRL_RELOAD | TIMCTRL_PRESCALE(0) |
			TIMCTRL_SELECT(0xb), timer_base + TIMCTRL1);
	imx28_cs.mult = clocksource_hz2mult(CLOCK_TICK_RATE, imx28_cs.shift);
	init_clock(&imx28_cs);

	return 0;
}

core_initcall(imx28_clocksource_init);
