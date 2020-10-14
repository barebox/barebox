/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Device driver for the Tegra 20 timer, which exposes a clocksource.
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/lowlevel.h>

/* register definitions */
#define TIMERUS_CNTR_1US	0x10
#define TIMERUS_USEC_CFG	0x14

static void __iomem *timer_base;

static uint64_t tegra20_timer_cs_read(void)
{
	return readl(timer_base + TIMERUS_CNTR_1US);
}

static struct clocksource cs = {
	.read	= tegra20_timer_cs_read,
	.mask	= CLOCKSOURCE_MASK(32),
};

static int tegra20_timer_probe(struct device_d *dev)
{
	struct resource *iores;
	u32 reg;

	/* use only one timer */
	if (timer_base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	timer_base = IOMEM(iores->start);

	/*
	 * calibrate timer to run at 1MHz
	 * TIMERUS_USEC_CFG selects the scale down factor with bits [0:7]
	 * representing the divisor and bits [8:15] representing the dividend
	 * each in n+1 form.
	 */
	switch (tegra_get_osc_clock()) {
	case 12000000:
		reg = 0x000b;
		break;
	case 13000000:
		reg = 0x000c;
		break;
	case 19200000:
		reg = 0x045f;
		break;
	case 26000000:
		reg = 0x0019;
		break;
	default:
		reg = 0;
		dev_warn(dev, "unknown timer clock rate\n");
		break;
	}
	writel(reg, timer_base + TIMERUS_USEC_CFG);

	cs.mult = clocksource_hz2mult(1000000, cs.shift);

	return init_clock(&cs);
}

static __maybe_unused struct of_device_id tegra20_timer_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d tegra20_timer_driver = {
	.probe	= tegra20_timer_probe,
	.name	= "tegra20-timer",
	.of_compatible = DRV_OF_COMPAT(tegra20_timer_dt_ids),
};

core_platform_driver(tegra20_timer_driver);
