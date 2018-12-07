/*
 * (C) Copyright 2009 Intel Corporation
 * Author: Jacob Pan (jacob.jun.pan@intel.com)
 *
 * Shared with ARM platforms, Jamie Iles, Picochip 2011
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Support for the Synopsys DesignWare APB Timers.
 *
 *
 * Taken from linux-4.9 kernel and adapted to barebox.
 */
#include <common.h>
#include <clock.h>
#include <init.h>

#include <linux/clk.h>
#include <linux/err.h>

#define APBT_MIN_PERIOD		4
#define APBT_MIN_DELTA_USEC		200

#define APBTMR_N_LOAD_COUNT		0x00
#define APBTMR_N_CURRENT_VALUE		0x04
#define APBTMR_N_CONTROL		0x08
#define APBTMR_N_EOI			0x0c
#define APBTMR_N_INT_STATUS		0x10

#define APBTMRS_INT_STATUS		0xa0
#define APBTMRS_EOI			0xa4
#define APBTMRS_RAW_INT_STATUS		0xa8
#define APBTMRS_COMP_VERSION		0xac

#define APBTMR_CONTROL_ENABLE		(1 << 0)
/* 1: periodic, 0:free running. */
#define APBTMR_CONTROL_MODE_PERIODIC	(1 << 1)
#define APBTMR_CONTROL_INT		(1 << 2)

#define APBTMRS_REG_SIZE		0x14

struct dw_apb_timer {
	void __iomem *base;
	unsigned long freq;
	int irq;
};

static struct dw_apb_timer timer;

static inline u32 apbt_readl(struct dw_apb_timer *timer, unsigned long offs)
{
	return readl(timer->base + offs);
}

static inline void apbt_writel(struct dw_apb_timer *timer, u32 val,
			unsigned long offs)
{
	writel(val, timer->base + offs);
}

/**
 * dw_apb_clocksource_start() - start the clocksource counting.
 *
 * @clksrc:	The clocksource to start.
 *
 * This is used to start the clocksource before registration and can be used
 * to enable calibration of timers.
 */
static int dw_apb_clocksource_start(struct clocksource *clksrc)
{
	/*
	 * start count down from 0xffff_ffff. this is done by toggling the
	 * enable bit then load initial load count to ~0.
	 */
	uint32_t ctrl = apbt_readl(&timer, APBTMR_N_CONTROL);

	ctrl &= ~APBTMR_CONTROL_ENABLE;
	apbt_writel(&timer, ctrl, APBTMR_N_CONTROL);
	apbt_writel(&timer, ~0, APBTMR_N_LOAD_COUNT);

	/* enable, mask interrupt */
	ctrl &= ~APBTMR_CONTROL_MODE_PERIODIC;
	ctrl |= (APBTMR_CONTROL_ENABLE | APBTMR_CONTROL_INT);
	apbt_writel(&timer, ctrl, APBTMR_N_CONTROL);

	return 0;
}

static uint64_t dw_apb_clocksource_read(void)
{
	return (uint64_t) ~apbt_readl(&timer, APBTMR_N_CURRENT_VALUE);
}

static struct clocksource dw_apb_clksrc = {
	.init  = dw_apb_clocksource_start,
	.read  = dw_apb_clocksource_read,
	.mask  = CLOCKSOURCE_MASK(32),
	.shift = 0,
};

static int dw_apb_timer_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct resource *iores;
	struct clk *clk;
	uint32_t clk_freq;

	/* use only one timer */
	if (timer.base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}

	timer.base = IOMEM(iores->start);

	/* Get clock frequency */
	clk = of_clk_get(np, 0);
	if (IS_ERR(clk)) {
		pr_err("Failed to get CPU clock: %ld\n", PTR_ERR(clk));
		return PTR_ERR(clk);
	}

	clk_freq = clk_get_rate(clk);
	clk_put(clk);

	dw_apb_clksrc.mult = clocksource_hz2mult(clk_freq, dw_apb_clksrc.shift);

	return init_clock(&dw_apb_clksrc);
}

static struct of_device_id dw_apb_timer_dt_ids[] = {
	{ .compatible = "snps,dw-apb-timer", },
	{ }
};

static struct driver_d dw_apb_timer_driver = {
	.name = "dw-apb-timer",
	.probe = dw_apb_timer_probe,
	.of_compatible = DRV_OF_COMPAT(dw_apb_timer_dt_ids),
};

device_platform_driver(dw_apb_timer_driver);
