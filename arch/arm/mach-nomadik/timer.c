/*
 * linux/arch/arm/mach-nomadik/timer.c
 *
 * Copyright (C) 2008 STMicroelectronics
 * Copyright (C) 2009 Alessandro Rubini, somewhat based on at91sam926x
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <init.h>
#include <clock.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/mtu.h>
#include <mach/timex.h>

/* Initial value for SRC control register: all timers use MXTAL/8 source */
#define SRC_CR_INIT_MASK	0x00007fff
#define SRC_CR_INIT_VAL		0x2aaa8000

static u32	nmdk_cycle;		/* write-once */
static __iomem void *mtu_base;

/*
 * clocksource: the MTU device is a decrementing counters, so we negate
 * the value being read.
 */
static uint64_t nmdk_read_timer(void)
{
	return nmdk_cycle - readl(mtu_base + MTU_VAL(0));
}

static struct clocksource nmdk_clksrc = {
	.read	= nmdk_read_timer,
	.shift	= 20,
	.mask	= CLOCKSOURCE_MASK(32),
};

static void nmdk_timer_reset(void)
{
	u32 cr;

	writel(0, mtu_base + MTU_CR(0)); /* off */

	/* configure load and background-load, and fire it up */
	writel(nmdk_cycle, mtu_base + MTU_LR(0));
	writel(nmdk_cycle, mtu_base + MTU_BGLR(0));
	cr = MTU_CRn_PRESCALE_1 | MTU_CRn_32BITS;
	writel(cr, mtu_base + MTU_CR(0));
	writel(cr | MTU_CRn_ENA, mtu_base + MTU_CR(0));
}

static int nmdk_timer_init(void)
{
	u32 src_cr;
	unsigned long rate;

	rate = CLOCK_TICK_RATE; /* 2.4MHz */
	nmdk_cycle = (rate + 1000 / 2) / 1000;

	/* Configure timer sources in "system reset controller" ctrl reg */
	src_cr = readl(NOMADIK_SRC_BASE);
	src_cr &= SRC_CR_INIT_MASK;
	src_cr |= SRC_CR_INIT_VAL;
	writel(src_cr, NOMADIK_SRC_BASE);

	/* Save global pointer to mtu, used by functions above */
	mtu_base = (void *)NOMADIK_MTU0_BASE;

	/* Init the timer and register clocksource */
	nmdk_timer_reset();

	nmdk_clksrc.mult = clocksource_hz2mult(rate, nmdk_clksrc.shift);

	init_clock(&nmdk_clksrc);

	return 0;
}
core_initcall(nmdk_timer_init);
