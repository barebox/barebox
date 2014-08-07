/*
 *  JZ4740 Watchdog driver
 *
 *  Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 *  Based on jz4740_wdt.c from linux-3.15.
 *
 *  Copyright (C) 2010, Paul Cercueil <paul@crapouillou.net>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>

#define JZ_REG_WDT_TIMER_DATA     0x0
#define JZ_REG_WDT_COUNTER_ENABLE 0x4
#define JZ_REG_WDT_TIMER_COUNTER  0x8
#define JZ_REG_WDT_TIMER_CONTROL  0xC

#define JZ_WDT_CLOCK_PCLK 0x1
#define JZ_WDT_CLOCK_RTC  0x2
#define JZ_WDT_CLOCK_EXT  0x4

#define JZ_WDT_CLOCK_DIV_SHIFT   3

#define JZ_WDT_CLOCK_DIV_1    (0 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_4    (1 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_16   (2 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_64   (3 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_256  (4 << JZ_WDT_CLOCK_DIV_SHIFT)
#define JZ_WDT_CLOCK_DIV_1024 (5 << JZ_WDT_CLOCK_DIV_SHIFT)

#define JZ_EXTAL 24000000

struct jz4740_wdt_drvdata {
	void __iomem *base;
};

static struct jz4740_wdt_drvdata *reset_wd;

void __noreturn reset_cpu(unsigned long addr)
{
	if (reset_wd) {
		void __iomem *base = reset_wd->base;

		writew(JZ_WDT_CLOCK_DIV_4 | JZ_WDT_CLOCK_EXT,
				base + JZ_REG_WDT_TIMER_CONTROL);
		writew(0, base + JZ_REG_WDT_TIMER_COUNTER);

		/* reset after 4ms */
		writew(JZ_EXTAL / 1000, base + JZ_REG_WDT_TIMER_DATA);

		/* start wdt */
		writeb(0x1, base + JZ_REG_WDT_COUNTER_ENABLE);

		mdelay(1000);
	} else
		pr_err("%s: can't reset cpu\n", __func__);

	hang();
}
EXPORT_SYMBOL(reset_cpu);

static int jz4740_wdt_probe(struct device_d *dev)
{
	struct jz4740_wdt_drvdata *priv;

	priv = xzalloc(sizeof(struct jz4740_wdt_drvdata));
	priv->base = dev_request_mem_region(dev, 0);
	if (!priv->base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	if (!reset_wd)
		reset_wd = priv;

	dev->priv = priv;

	return 0;
}

static __maybe_unused struct of_device_id jz4740_wdt_dt_ids[] = {
	{
		.compatible = "ingenic,jz4740-wdt",
	}, {
		/* sentinel */
	}
};

static struct driver_d jz4740_wdt_driver = {
	.name   = "jz4740-wdt",
	.probe  = jz4740_wdt_probe,
	.of_compatible = DRV_OF_COMPAT(jz4740_wdt_dt_ids),
};
device_platform_driver(jz4740_wdt_driver);
