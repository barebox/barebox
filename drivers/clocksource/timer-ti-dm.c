// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * @brief Support DMTimer counter
 *
 * FileName: arch/arm/mach-omap/dmtimer.c
 */
/*
 * This File is based on arch/arm/mach-omap/s32k_clksource.c
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * (C) Copyright 2012 Phytec Messtechnik GmbH
 * Author: Teresa Gámez <t.gamez@phytec.de>
 * (C) Copyright 2015 Phytec Messtechnik GmbH
 * Author: Daniel Schultz <d.schultz@phytec.de>
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/omap/am33xx-clock.h>
#include <linux/clk.h>

#include <stdio.h>

#define CLK_RC32K	32768

#define TIDR			0x0
#define TIOCP_CFG		0x10
#define IRQ_EOI			0x20
#define IRQSTATUS_RAW		0x24
#define IRQSTATUS		0x28
#define IRQSTATUS_SET		0x2c
#define IRQSTATUS_CLR		0x30
#define IRQWAKEEN		0x34
#define TCLR			0x38
#define TCRR			0x3C
#define TLDR			0x40
#define TTGR			0x44
#define TWPS			0x48
#define TMAR			0x4C
#define TCAR1			0x50
#define TSICR			0x54
#define TCAR2			0x58

static void *base;

/**
 * @brief Provide a simple counter read
 *
 * @return DMTimer counter
 */
static uint64_t dmtimer_read(void)
{
	return readl(base + TCRR);
}

static struct clocksource dmtimer_cs = {
	.read	= dmtimer_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
	.priority = 70,
};

struct omap_dmtimer_data {
	int (*get_clock)(struct device *dev);
};

int omap_dmtimer_init(void __iomem *mmio_start, unsigned clk_speed)
{
	base = mmio_start;

	dmtimer_cs.mult = clocksource_hz2mult(clk_speed, dmtimer_cs.shift);

	/* Enable counter */
	writel(0x3, base + TCLR);

	return init_clock(&dmtimer_cs);
}

static int omap_dmtimer_probe(struct device *dev)
{
	struct resource *iores;
	int clk_speed;
	const struct omap_dmtimer_data *data;

	/* one timer is enough */
	if (base)
		return 0;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	data = device_get_match_data(dev);

	clk_speed = data->get_clock(dev);
	if (clk_speed < 0)
		return clk_speed;

	return omap_dmtimer_init(IOMEM(iores->start), clk_speed);
}

static int am335x_get_clock(struct device *dev)
{
	return am33xx_get_osc_clock() * 1000;
}

static __maybe_unused struct omap_dmtimer_data am335x_data = {
	.get_clock = am335x_get_clock,
};

static int k3_get_clock(struct device *dev)
{
	struct clk *clk;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "Cannot get clock\n");

	return clk_get_rate(clk);
}

static __maybe_unused struct omap_dmtimer_data k3_data = {
	.get_clock = k3_get_clock,
};

static __maybe_unused struct of_device_id omap_dmtimer_dt_ids[] = {
#ifdef CONFIG_ARCH_OMAP
	{
		.compatible = "ti,am335x-timer",
		.data = &am335x_data,
	},
#endif
#ifdef CONFIG_ARCH_K3
	{
		.compatible = "ti,am654-timer",
		.data = &k3_data,
	},
#endif
	{
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, omap_dmtimer_dt_ids);

static struct driver omap_dmtimer_driver = {
	.name = "omap-dmtimer",
	.probe = omap_dmtimer_probe,
	.of_compatible = DRV_OF_COMPAT(omap_dmtimer_dt_ids),
};

postcore_platform_driver(omap_dmtimer_driver);
