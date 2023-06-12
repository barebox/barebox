// SPDX-License-Identifier: GPL-2.0-or-later
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
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/omap/omap3-silicon.h>
#include <mach/omap/omap4-silicon.h>
#include <mach/omap/clocks.h>
#include <mach/omap/timers.h>
#include <mach/omap/sys_info.h>
#include <mach/omap/syslib.h>

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
	.priority = 70,
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
static int omap_32ktimer_probe(struct device *dev)
{
	struct resource *iores;

	/* one timer is enough */
	if (timerbase)
		return 0;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	timerbase = IOMEM(iores->start);

	s32k_cs.mult = clocksource_hz2mult(S32K_FREQUENCY, s32k_cs.shift);

	return init_clock(&s32k_cs);
}

static __maybe_unused struct of_device_id omap_32ktimer_dt_ids[] = {
	{
		.compatible = "ti,omap-counter32k",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, omap_32ktimer_dt_ids);

static struct driver omap_32ktimer_driver = {
	.name = "omap-32ktimer",
	.probe = omap_32ktimer_probe,
	.of_compatible = DRV_OF_COMPAT(omap_32ktimer_dt_ids),
};

postcore_platform_driver(omap_32ktimer_driver);
