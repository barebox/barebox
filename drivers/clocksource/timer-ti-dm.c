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
 *
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>

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
};

static int omap_dmtimer_probe(struct device_d *dev)
{
	struct resource *iores;
	u64 clk_speed;

	/* one timer is enough */
	if (base)
		return 0;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	clk_speed = am33xx_get_osc_clock();
	clk_speed *= 1000;
	dmtimer_cs.mult = clocksource_hz2mult(clk_speed, dmtimer_cs.shift);

	/* Enable counter */
	writel(0x3, base + TCLR);

	return init_clock(&dmtimer_cs);
}

static __maybe_unused struct of_device_id omap_dmtimer_dt_ids[] = {
	{
		.compatible = "ti,am335x-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d omap_dmtimer_driver = {
	.name = "omap-dmtimer",
	.probe = omap_dmtimer_probe,
	.of_compatible = DRV_OF_COMPAT(omap_dmtimer_dt_ids),
};

static int omap_dmtimer_init(void)
{
	return platform_driver_register(&omap_dmtimer_driver);
}
postcore_initcall(omap_dmtimer_init);
