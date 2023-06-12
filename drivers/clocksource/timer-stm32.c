// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 * Author(s): Patrice Chotard, <patrice.chotard@foss.st.com> for STMicroelectronics.
 */

#include <common.h>
#include <linux/clk.h>
#include <clock.h>
#include <init.h>
#include <io.h>

/* Timer control1 register  */
#define CR1_CEN			BIT(0)
#define CR1_ARPE		BIT(7)

/* Event Generation Register register  */
#define EGR_UG			BIT(0)

/* Auto reload register for free running config */
#define GPT_FREE_RUNNING	0xFFFFFFFF

#define MHZ_1			1000000

struct stm32_timer_regs {
	u32 cr1;
	u32 cr2;
	u32 smcr;
	u32 dier;
	u32 sr;
	u32 egr;
	u32 ccmr1;
	u32 ccmr2;
	u32 ccer;
	u32 cnt;
	u32 psc;
	u32 arr;
	u32 reserved;
	u32 ccr1;
	u32 ccr2;
	u32 ccr3;
	u32 ccr4;
	u32 reserved1;
	u32 dcr;
	u32 dmar;
	u32 tim2_5_or;
};

static struct stm32_timer_regs *timer_base;

static u64 stm32_timer_read(void)
{
	return readl(&timer_base->cnt);
}

/* A bit obvious isn't it? */
static struct clocksource cs = {
	.read = stm32_timer_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift = 0,
	.priority = 100,
};

static int stm32_timer_probe(struct device *dev)
{
	struct resource *iores;
	struct clk *clk;
	u32 rate, psc;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	timer_base = IOMEM(iores->start);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_enable(clk);
	if (ret)
		return ret;

	/* Stop the timer */
	clrbits_le32(&timer_base->cr1, CR1_CEN);

	/* get timer clock */
	rate = clk_get_rate(clk);

	/* we set timer prescaler to obtain a 1MHz timer counter frequency */
	psc = (rate / MHZ_1) - 1;
	writel(psc, &timer_base->psc);

	/* Configure timer for auto-reload */
	setbits_le32(&timer_base->cr1, CR1_ARPE);

	/* load value for auto reload */
	writel(GPT_FREE_RUNNING, &timer_base->arr);

	/* start timer */
	setbits_le32(&timer_base->cr1, CR1_CEN);

	/* Update generation */
	setbits_le32(&timer_base->egr, EGR_UG);

	cs.mult = clocksource_hz2mult(MHZ_1, cs.shift);

	return init_clock(&cs);
}

static struct of_device_id stm32_timer_dt_ids[] = {
	{ .compatible = "st,stm32-timer" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stm32_timer_dt_ids);

static struct driver stm32_timer_driver = {
	.name = "stm32-timer",
	.probe = stm32_timer_probe,
	.of_compatible = stm32_timer_dt_ids,
};
postcore_platform_driver(stm32_timer_driver);
