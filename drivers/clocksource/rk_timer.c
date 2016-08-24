/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * (C) Copyright 2016 PHYTEC Messtechnik GmbH
 * Author: Wadim Egorov <w.egorov@phytec.de>

 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/timer.h>
#include <stdio.h>
#include <mach/hardware.h>
#include <mach/cru_rk3288.h>
#include <common.h>

struct rk_timer *timer_ptr;

static uint64_t rockchip_get_ticks(void)
{
	uint64_t timebase_h, timebase_l;

	timebase_l = readl(&timer_ptr->timer_curr_value0);
	timebase_h = readl(&timer_ptr->timer_curr_value1);

	return timebase_h << 32 | timebase_l;
}

static struct clocksource rkcs = {
	.read   = rockchip_get_ticks,
	.mask   = CLOCKSOURCE_MASK(32),
	.shift  = 10,
};

static int rockchip_timer_probe(struct device_d *dev)
{
	struct resource *iores;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	timer_ptr = IOMEM(iores->start) + 0x20 * CONFIG_RK_TIMER;

	rkcs.mult = clocksource_hz2mult(OSC_HZ, rkcs.shift);

	writel(0xffffffff, &timer_ptr->timer_load_count0);
	writel(0xffffffff, &timer_ptr->timer_load_count1);
	writel(1, &timer_ptr->timer_ctrl_reg);

	return init_clock(&rkcs);
}

static __maybe_unused struct of_device_id rktimer_dt_ids[] = {
	{
		.compatible = "rockchip,rk3288-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d rktimer_driver = {
	.name = "rockchip-timer",
	.probe = rockchip_timer_probe,
	.of_compatible = DRV_OF_COMPAT(rktimer_dt_ids),
};

static int rktimer_init(void)
{
	return platform_driver_register(&rktimer_driver);
}
core_initcall(rktimer_init);
