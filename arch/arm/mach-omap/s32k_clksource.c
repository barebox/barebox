/**
 * @file
 * @brief Provide @ref clocksource functionality for OMAP
 *
 * @ref clocksource provides a neat architecture. all we do is
 * To loop in with Sync 32Khz clock ticking away at 32768hz on OMAP.
 * Sync 32Khz clock is an always ON clock.
 *
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/omap3-silicon.h>
#include <mach/omap4-silicon.h>
#include <mach/clocks.h>
#include <mach/timers.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>

/** Sync 32Khz Timer registers */
#define S32K_CR			0x10
#define S32K_FREQUENCY		32768

static void __iomem *timerbase;

/**
 * @brief Provide a simple clock read
 *
 * Nothing is simpler.. read direct from clock and provide it
 * to the caller.
 *
 * @return S32K clock counter
 */
static uint64_t s32k_clocksource_read(void)
{
	return readl(timerbase + S32K_CR);
}

/* A bit obvious isn't it? */
static struct clocksource s32k_cs = {
	.read = s32k_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift = 10,
};

/**
 * @brief Initialize the Clock
 *
 * There is nothing to do on OMAP as SYNC32K clock is
 * always on, and we do a simple data structure initialization.
 * 32K clock gives 32768 ticks a seconds
 *
 * @return result of @ref init_clock
 */
static int s32k_clocksource_init(void)
{
	if (IS_ENABLED(CONFIG_ARCH_OMAP3))
		timerbase = (void *)OMAP3_32KTIMER_BASE;
	else if (IS_ENABLED(CONFIG_ARCH_OMAP4))
		timerbase = (void *)OMAP44XX_32KTIMER_BASE;
	else
		BUG();

	s32k_cs.mult = clocksource_hz2mult(S32K_FREQUENCY, s32k_cs.shift);

	return init_clock(&s32k_cs);
}

/* Run me at boot time */
core_initcall(s32k_clocksource_init);
