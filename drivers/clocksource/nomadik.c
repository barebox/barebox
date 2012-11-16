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
#include <clock.h>
#include <io.h>
#include <init.h>
#include <driver.h>
#include <errno.h>
#include <linux/clk.h>
#include <linux/err.h>

/*
 * The MTU device hosts four different counters, with 4 set of
 * registers. These are register names.
 */

#define MTU_IMSC	0x00	/* Interrupt mask set/clear */
#define MTU_RIS		0x04	/* Raw interrupt status */
#define MTU_MIS		0x08	/* Masked interrupt status */
#define MTU_ICR		0x0C	/* Interrupt clear register */

/* per-timer registers take 0..3 as argument */
#define MTU_LR(x)	(0x10 + 0x10 * (x) + 0x00)	/* Load value */
#define MTU_VAL(x)	(0x10 + 0x10 * (x) + 0x04)	/* Current value */
#define MTU_CR(x)	(0x10 + 0x10 * (x) + 0x08)	/* Control reg */
#define MTU_BGLR(x)	(0x10 + 0x10 * (x) + 0x0c)	/* At next overflow */

/* bits for the control register */
#define MTU_CRn_ENA		0x80
#define MTU_CRn_PERIODIC	0x40	/* if 0 = free-running */
#define MTU_CRn_PRESCALE_MASK	0x0c
#define MTU_CRn_PRESCALE_1		0x00
#define MTU_CRn_PRESCALE_16		0x04
#define MTU_CRn_PRESCALE_256		0x08
#define MTU_CRn_32BITS		0x02
#define MTU_CRn_ONESHOT		0x01	/* if 0 = wraps reloading from BGLR*/

/* Other registers are usual amba/primecell registers, currently not used */
#define MTU_ITCR	0xff0
#define MTU_ITOP	0xff4

#define MTU_PERIPH_ID0	0xfe0
#define MTU_PERIPH_ID1	0xfe4
#define MTU_PERIPH_ID2	0xfe8
#define MTU_PERIPH_ID3	0xfeC

#define MTU_PCELL0	0xff0
#define MTU_PCELL1	0xff4
#define MTU_PCELL2	0xff8
#define MTU_PCELL3	0xffC

static __iomem void *mtu_base;
static u32 clk_prescale;
static u32 nmdk_cycle;		/* write-once */

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

	/* Disable */
	writel(0, mtu_base + MTU_CR(0)); /* off */

	/* configure load and background-load, and fire it up */
	writel(nmdk_cycle, mtu_base + MTU_LR(0));
	writel(nmdk_cycle, mtu_base + MTU_BGLR(0));

	cr = clk_prescale | MTU_CRn_32BITS;
	writel(cr, mtu_base + MTU_CR(0));
	writel(cr | MTU_CRn_ENA, mtu_base + MTU_CR(0));
}

static int nmdk_mtu_probe(struct device_d *dev)
{
	static struct clk *mtu_clk;
	u32 rate;
	int ret;

	mtu_clk = clk_get(dev, NULL);
	if (IS_ERR(mtu_clk)) {
		ret = PTR_ERR(mtu_clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(mtu_clk);
	if (ret < 0) {
		dev_err(dev, "clock failed to enable: %d\n", ret);
		clk_put(mtu_clk);
		return ret;
	}

	rate = clk_get_rate(mtu_clk);
	if (rate > 32000000) {
		rate /= 16;
		clk_prescale = MTU_CRn_PRESCALE_16;
	} else {
		clk_prescale = MTU_CRn_PRESCALE_1;
	}

	nmdk_cycle = (rate + 1000 / 2) / 1000;

	/* Save global pointer to mtu, used by functions above */
	mtu_base = dev_request_mem_region(dev, 0);

	/* Init the timer and register clocksource */
	nmdk_timer_reset();

	nmdk_clksrc.mult = clocksource_hz2mult(rate, nmdk_clksrc.shift);

	init_clock(&nmdk_clksrc);

	return 0;
}

static struct driver_d nmdk_mtu_driver = {
	.name = "nomadik_mtu",
	.probe = nmdk_mtu_probe,
};

static int nmdk_mtu_init(void)
{
	return platform_driver_register(&nmdk_mtu_driver);
}
coredevice_initcall(nmdk_mtu_init);
