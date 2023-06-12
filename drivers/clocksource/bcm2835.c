// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: Carlo Caione <carlo@carlocaione.org>
 *
 * Based on clocksource for nomadik
 */


#include <common.h>
#include <clock.h>
#include <io.h>
#include <init.h>
#include <driver.h>
#include <errno.h>
#include <linux/clk.h>
#include <linux/err.h>

/* Registers */
#define ST_CLO	0x04

static __iomem void *stc_base;

static uint64_t stc_read_cycles(void)
{
	return readl(stc_base + ST_CLO);
}

static struct clocksource bcm2835_stc = {
	.read = stc_read_cycles,
	.mask = CLOCKSOURCE_MASK(32),
	/* Give the architected timer precedence on AArch64 */
	.priority = IS_ENABLED(CONFIG_CPU_V8) ? 60 : 80,
};

static int bcm2835_cs_probe(struct device *dev)
{
	struct resource *iores;
	static struct clk *stc_clk;
	u32 rate = 0;
	int ret;

	/* try to read rate from DT property first */
	if (IS_ENABLED(CONFIG_OFTREE))
		of_property_read_u32(dev->of_node, "clock-frequency",
				     &rate);

	/* if rate is still empty, try to get rate from clk */
	if (!rate) {
		stc_clk = clk_get(dev, NULL);
		if (IS_ERR(stc_clk)) {
			ret = PTR_ERR(stc_clk);
			dev_err(dev, "clock not found: %d\n", ret);
			return ret;
		}

		ret = clk_enable(stc_clk);
		if (ret) {
			dev_err(dev, "clock failed to enable: %d\n", ret);
			clk_put(stc_clk);
			return ret;
		}

		rate = clk_get_rate(stc_clk);
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	stc_base = IOMEM(iores->start);

	clocks_calc_mult_shift(&bcm2835_stc.mult, &bcm2835_stc.shift, rate, NSEC_PER_SEC, 60);

	return init_clock(&bcm2835_stc);
}

static __maybe_unused struct of_device_id bcm2835_cs_dt_ids[] = {
	{
		.compatible = "brcm,bcm2835-system-timer",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, bcm2835_cs_dt_ids);

static struct driver bcm2835_cs_driver = {
	.name = "bcm2835-cs",
	.probe = bcm2835_cs_probe,
	.of_compatible = DRV_OF_COMPAT(bcm2835_cs_dt_ids),
};

core_platform_driver(bcm2835_cs_driver);
