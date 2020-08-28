/*
 * Copyright (C) 2013, 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
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

#include <common.h>
#include <io.h>
#include <init.h>
#include <clock.h>
#include <linux/err.h>

#define DIGIC_TIMER_CLOCK 1000000

#define DIGIC_TIMER_CONTROL 0x00
#define DIGIC_TIMER_VALUE 0x0c

static void __iomem *timer_base;

static uint64_t digic_cs_read(void)
{
	return (uint64_t)(0xffff - readl(timer_base + DIGIC_TIMER_VALUE));
}

static struct clocksource digic_cs = {
	.read	= digic_cs_read,
	.mask   = CLOCKSOURCE_MASK(16),
};

static int digic_timer_probe(struct device_d *dev)
{
	struct resource *iores;
	/* use only one timer */
	if (timer_base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	timer_base = IOMEM(iores->start);

	clocks_calc_mult_shift(&digic_cs.mult, &digic_cs.shift,
		DIGIC_TIMER_CLOCK, NSEC_PER_SEC, 1);

	/* disable timer */
	writel(0x80000000, timer_base + DIGIC_TIMER_CONTROL);

	/* magic values... divider? */
	writel(0x00000002, timer_base + 0x04);
	writel(0x00000003, timer_base + 0x14);

	/* max counter value */
	writel(0x0000ffff, timer_base + 0x08);

	init_clock(&digic_cs);

	/* enable timer */
	writel(0x00000001, timer_base + DIGIC_TIMER_CONTROL);
	/* start timer */
	writel(0x00000001, timer_base + 0x10);

	return 0;
}

static __maybe_unused struct of_device_id digic_timer_dt_ids[] = {
	{
		.compatible = "canon,digic-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d digic_timer_driver = {
	.probe	= digic_timer_probe,
	.name	= "digic-timer",
	.of_compatible = DRV_OF_COMPAT(digic_timer_dt_ids),
};

coredevice_platform_driver(digic_timer_driver);
