/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Based on
 * https://github.com/RC-MODULE/linux-3.10.x/tree/k1879-3.10.28/arch/arm/mach-uemd/clocksource.c
 * (C) 2011 RC Module, Sergey Mironov <ierton@gmail.com>
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
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <clock.h>

#define TIMER_LOAD		0x00
#define TIMER_VALUE		0x04
#define TIMER_CONTROL		0x08

#define TIMER_CTRL_ENABLE	BIT(7)

/*
 * TIMER_CONTROL_PERIODIC:
 * The counter generates an interrupt at a constant interval,
 * reloading the original value after wrapping past zero.
 */
#define TIMER_CTRL_PERIODIC	BIT(6)

/* interrupt enable */
#define TIMER_CTRL_IE		BIT(5)

/* Prescalers */
#define TIMER_CTRL_P1		(0 << 2)
#define TIMER_CTRL_P16		(1 << 2)
#define TIMER_CTRL_P256		(2 << 2)

/* Counter register size is 32 Bit long */
#define TIMER_CTRL_32BIT	BIT(1)

static void __iomem *timer_base;

static uint64_t uemd_timer_cs_read(void)
{
	/* Down counter! */
	return ~__raw_readl(timer_base + TIMER_VALUE);
}

static struct clocksource uemd_cs = {
	.read = uemd_timer_cs_read,
	.mask = CLOCKSOURCE_MASK(32),
};

static int uemd_timer_probe(struct device_d *dev)
{
	struct resource *iores;
	int mode;
	struct clk *timer_clk;

	/* use only one timer */
	if (timer_base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	timer_base = IOMEM(iores->start);

	timer_clk = clk_get(dev, NULL);
	if (IS_ERR(timer_clk)) {
		int ret = PTR_ERR(timer_clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	/* Stop timer */
	__raw_writel(0, timer_base + TIMER_CONTROL);

	/* Setup */
	__raw_writel(0xffffffff, timer_base + TIMER_LOAD);
	__raw_writel(0xffffffff, timer_base + TIMER_VALUE);

	mode =	TIMER_CTRL_32BIT	|
		TIMER_CTRL_PERIODIC	|
		TIMER_CTRL_P1;
	__raw_writel(mode, timer_base + TIMER_CONTROL);

	/* Fire it up! */
	mode |= TIMER_CTRL_ENABLE;
	__raw_writel(mode, timer_base + TIMER_CONTROL);

	clocks_calc_mult_shift(&uemd_cs.mult, &uemd_cs.shift,
		clk_get_rate(timer_clk), NSEC_PER_SEC, 10);

	return init_clock(&uemd_cs);
}

static __maybe_unused struct of_device_id uemd_timer_dt_ids[] = {
	{
		.compatible = "module,uemd-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d uemd_timer_driver = {
	.probe	= uemd_timer_probe,
	.name	= "uemd-timer",
	.of_compatible = DRV_OF_COMPAT(uemd_timer_dt_ids),
};

coredevice_platform_driver(uemd_timer_driver);
