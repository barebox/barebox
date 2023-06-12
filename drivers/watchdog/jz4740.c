// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  JZ4740 Watchdog driver
 *
 *  Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 *  Based on jz4740_wdt.c from linux-3.15.
 *
 *  Copyright (C) 2010, Paul Cercueil <paul@crapouillou.net>
 */

#include <common.h>
#include <init.h>
#include <restart.h>
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
	struct restart_handler restart;
	void __iomem *base;
};

static void __noreturn jz4740_reset_soc(struct restart_handler *rst)
{
	struct jz4740_wdt_drvdata *priv =
		container_of(rst, struct jz4740_wdt_drvdata, restart);
	void __iomem *base = priv->base;

	writew(JZ_WDT_CLOCK_DIV_4 | JZ_WDT_CLOCK_EXT,
			base + JZ_REG_WDT_TIMER_CONTROL);
	writew(0, base + JZ_REG_WDT_TIMER_COUNTER);

	/* reset after 4ms */
	writew(JZ_EXTAL / 1000, base + JZ_REG_WDT_TIMER_DATA);

	/* start wdt */
	writeb(0x1, base + JZ_REG_WDT_COUNTER_ENABLE);

	mdelay(1000);

	hang();
}

static int jz4740_wdt_probe(struct device *dev)
{
	struct resource *iores;
	struct jz4740_wdt_drvdata *priv;

	priv = xzalloc(sizeof(struct jz4740_wdt_drvdata));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	priv->base = IOMEM(iores->start);

	dev->priv = priv;

	priv->restart.name = "jz4740-wdt";
	priv->restart.restart = jz4740_reset_soc;
	restart_handler_register(&priv->restart);

	return 0;
}

static __maybe_unused struct of_device_id jz4740_wdt_dt_ids[] = {
	{
		.compatible = "ingenic,jz4740-wdt",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, jz4740_wdt_dt_ids);

static struct driver jz4740_wdt_driver = {
	.name   = "jz4740-wdt",
	.probe  = jz4740_wdt_probe,
	.of_compatible = DRV_OF_COMPAT(jz4740_wdt_dt_ids),
};
device_platform_driver(jz4740_wdt_driver);
