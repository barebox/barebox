// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/watchdog/meson_gxbb_wdt.c?id=b152ca999c5f11fefdd156a1182e022fa22ebed3
/*
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <of.h>
#include <of_device.h>
#include <driver.h>
#include <clock.h>
#include <linux/slab.h>
#include <linux/stringify.h>
#include <linux/types.h>
#include <watchdog.h>

#define GXBB_WDT_CTRL_REG			0x0
#define GXBB_WDT_TCNT_REG			0x8
#define GXBB_WDT_RSET_REG			0xc

#define GXBB_WDT_CTRL_CLKDIV_EN			BIT(25)
#define GXBB_WDT_CTRL_CLK_EN			BIT(24)
#define GXBB_WDT_CTRL_EN			BIT(18)
#define GXBB_WDT_CTRL_DIV_MASK			(BIT(18) - 1)

#define GXBB_WDT_TCNT_SETUP_MASK		(BIT(16) - 1)
#define GXBB_WDT_TCNT_CNT_SHIFT			16

struct meson_gxbb_wdt {
	void __iomem *reg_base;
	struct watchdog wdt_dev;
	struct clk *clk;
};

struct wdt_params {
	u32 rst;
};

static int meson_gxbb_wdt_start(struct watchdog *wdt_dev)
{
	struct meson_gxbb_wdt *data =
		container_of(wdt_dev, struct meson_gxbb_wdt, wdt_dev);

	writel(readl(data->reg_base + GXBB_WDT_CTRL_REG) | GXBB_WDT_CTRL_EN,
	       data->reg_base + GXBB_WDT_CTRL_REG);

	return 0;
}

static int meson_gxbb_wdt_stop(struct watchdog *wdt_dev)
{
	struct meson_gxbb_wdt *data =
		container_of(wdt_dev, struct meson_gxbb_wdt, wdt_dev);

	writel(readl(data->reg_base + GXBB_WDT_CTRL_REG) & ~GXBB_WDT_CTRL_EN,
	       data->reg_base + GXBB_WDT_CTRL_REG);

	return 0;
}

static int meson_gxbb_wdt_ping(struct watchdog *wdt_dev)
{
	struct meson_gxbb_wdt *data =
		container_of(wdt_dev, struct meson_gxbb_wdt, wdt_dev);

	writel(0, data->reg_base + GXBB_WDT_RSET_REG);

	return 0;
}

static int meson_gxbb_wdt_set_timeout(struct watchdog *wdt_dev,
				      unsigned int timeout)
{
	struct meson_gxbb_wdt *data =
		container_of(wdt_dev, struct meson_gxbb_wdt, wdt_dev);
	unsigned long tcnt = timeout * 1000;

	if (!timeout)
		return meson_gxbb_wdt_stop(wdt_dev);

	if (tcnt > GXBB_WDT_TCNT_SETUP_MASK)
		tcnt = GXBB_WDT_TCNT_SETUP_MASK;

	meson_gxbb_wdt_ping(wdt_dev);

	writel(tcnt, data->reg_base + GXBB_WDT_TCNT_REG);

	return meson_gxbb_wdt_start(wdt_dev);
}

static const struct wdt_params gxbb_params = {
	.rst = BIT(21),
};

static const struct wdt_params t7_params = {
	.rst = BIT(22),
};

static const struct of_device_id meson_gxbb_wdt_dt_ids[] = {
	 { .compatible = "amlogic,meson-gxbb-wdt", .data = &gxbb_params, },
	 { .compatible = "amlogic,t7-wdt", .data = &t7_params, },
	 { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, meson_gxbb_wdt_dt_ids);

static int meson_gxbb_wdt_probe(struct device *dev)
{
	struct meson_gxbb_wdt *data;
	struct wdt_params *params;
	u32 ctrl_reg;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->reg_base = dev_platform_ioremap_resource(dev, 0);
	if (IS_ERR(data->reg_base))
		return PTR_ERR(data->reg_base);

	data->clk = clk_get_enabled(dev, NULL);
	if (IS_ERR(data->clk))
		return PTR_ERR(data->clk);

	params = (struct wdt_params *)of_device_get_match_data(dev);

	dev_set_drvdata(dev, data);

	data->wdt_dev.hwdev = dev;
	data->wdt_dev.set_timeout = meson_gxbb_wdt_set_timeout;
	data->wdt_dev.ping = meson_gxbb_wdt_ping;
	data->wdt_dev.timeout_max = GXBB_WDT_TCNT_SETUP_MASK / 1000;

	ctrl_reg = readl(data->reg_base + GXBB_WDT_CTRL_REG) &
				GXBB_WDT_CTRL_EN;

	data->wdt_dev.running = ctrl_reg ? WDOG_HW_RUNNING : WDOG_HW_NOT_RUNNING;

	if (ctrl_reg) {
		/* Watchdog is running - keep it running but extend timeout
		 * to the maximum while setting the timebase
		 */
		data->wdt_dev.timeout_cur = data->wdt_dev.timeout_max;
		data->wdt_dev.last_ping = get_time_ns();
		meson_gxbb_wdt_set_timeout(&data->wdt_dev,
				GXBB_WDT_TCNT_SETUP_MASK / 1000);
	}

	/* Setup with 1ms timebase */
	ctrl_reg |= ((clk_get_rate(data->clk) / 1000) &
			GXBB_WDT_CTRL_DIV_MASK) |
			params->rst |
			GXBB_WDT_CTRL_CLK_EN |
			GXBB_WDT_CTRL_CLKDIV_EN;

	writel(ctrl_reg, data->reg_base + GXBB_WDT_CTRL_REG);

	return watchdog_register(&data->wdt_dev);
}

static struct driver meson_gxbb_wdt_driver = {
	.probe	= meson_gxbb_wdt_probe,
	.name = "meson-gxbb-wdt",
	.of_match_table = meson_gxbb_wdt_dt_ids,
};

device_platform_driver(meson_gxbb_wdt_driver);

MODULE_AUTHOR("Neil Armstrong <narmstrong@baylibre.com>");
MODULE_DESCRIPTION("Amlogic Meson GXBB Watchdog timer driver");
MODULE_LICENSE("Dual BSD/GPL");
