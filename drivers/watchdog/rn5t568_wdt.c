// SPDX-License-Identifier: GPL-2.0-only
/*
 * Watchdog driver for Ricoh RN5T618 PMIC
 *
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 */

#include <common.h>
#include <init.h>
#include <watchdog.h>
#include <regmap.h>
#include <of.h>

#define RN5T568_WATCHDOG 0x0b
# define RN5T568_WATCHDOG_WDPWROFFEN BIT(2)
# define RN5T568_WATCHDOG_WDOGTIM_M (BIT(0) | BIT(1))
#define RN5T568_PWRIREN 0x12
# define RN5T568_PWRIREN_EN_WDOG BIT(6)
#define RN5T568_PWRIRQ 0x13
# define RN5T568_PWRIRQ_IR_WDOG BIT(6)

struct rn5t568_wdt {
	struct watchdog wdd;
	struct regmap *regmap;
	unsigned int timeout;
};

struct rn5t568_wdt_tim {
	u8 reg_val;
	u8 time;
};

static const struct rn5t568_wdt_tim rn5t568_wdt_timeout[] = {
	{ .reg_val = 0, .time = 1, },
	{ .reg_val = 1, .time = 8, },
	{ .reg_val = 2, .time = 32, },
	{ .reg_val = 3, .time = 128, },
};

#define PMIC_WDT_MAX_TIMEOUT 128

static int rn5t568_wdt_start(struct regmap *regmap, int idx)
{
	int ret;

	ret = regmap_update_bits(regmap, RN5T568_WATCHDOG, RN5T568_WATCHDOG_WDOGTIM_M,
				 rn5t568_wdt_timeout[idx].reg_val);
	if (ret)
		return ret;

	regmap_clear_bits(regmap, RN5T568_PWRIRQ, RN5T568_PWRIRQ_IR_WDOG);
	regmap_set_bits(regmap, RN5T568_PWRIREN, RN5T568_PWRIREN_EN_WDOG);

	pr_debug("RN5t: Starting the watchdog with %u seconds\n", rn5t568_wdt_timeout[idx].time);

	return regmap_set_bits(regmap, RN5T568_WATCHDOG, RN5T568_WATCHDOG_WDPWROFFEN);
}

static int rn5t568_wdt_stop(struct regmap *regmap)
{
	int ret;

	ret  = regmap_clear_bits(regmap, RN5T568_PWRIREN, RN5T568_PWRIREN_EN_WDOG);
	if (ret)
		return ret;

	return regmap_clear_bits(regmap, RN5T568_WATCHDOG, RN5T568_WATCHDOG_WDPWROFFEN);
}

static int rn5t568_wdt_ping(struct regmap *regmap)
{
	unsigned int val;
	int ret;

	ret = regmap_read(regmap, RN5T568_WATCHDOG, &val);
	if (ret)
		return ret;

	return regmap_write(regmap, RN5T568_WATCHDOG, val);
}

static int rn5t568_wdt_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct rn5t568_wdt *wdt = container_of(wdd, struct rn5t568_wdt, wdd);
	int ret, i;

	if (!timeout)
		return rn5t568_wdt_stop(wdt->regmap);

	for (i = 0; i < ARRAY_SIZE(rn5t568_wdt_timeout); i++) {
		if (timeout < rn5t568_wdt_timeout[i].time)
			break;
	}

	if (i == ARRAY_SIZE(rn5t568_wdt_timeout))
		return -EINVAL;

	if (wdt->timeout == timeout)
		return rn5t568_wdt_ping(wdt->regmap);

	ret = rn5t568_wdt_start(wdt->regmap, i);
	if (ret)
		return ret;

	wdt->timeout = rn5t568_wdt_timeout[i].time;

	return ret;
}

static int rn5t568_wdt_probe(struct device *dev)
{
	struct rn5t568_wdt *wdt;
	struct watchdog *wdd;
	unsigned int val;
	int ret;

	wdt = xzalloc(sizeof(*wdt));

	wdt->regmap = dev_get_regmap(dev->parent, NULL);
	if (IS_ERR(wdt->regmap))
		return PTR_ERR(wdt->regmap);

	wdd = &wdt->wdd;
	wdd->hwdev = dev;
	wdd->set_timeout = rn5t568_wdt_set_timeout;
	wdd->timeout_max = PMIC_WDT_MAX_TIMEOUT;

	ret = regmap_read(wdt->regmap, RN5T568_WATCHDOG, &val);
	if (ret == 0)
		wdd->running = val & RN5T568_WATCHDOG_WDPWROFFEN ?
			       WDOG_HW_RUNNING : WDOG_HW_NOT_RUNNING;

	return watchdog_register(wdd);
}

static __maybe_unused const struct of_device_id rn5t568_wdt_of_match[] = {
	{ .compatible = "ricoh,rn5t568-wdt" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rn5t568_wdt_of_match);

static struct driver rn5t568_wdt_driver = {
	.name  = "rn5t568-wdt",
	.probe = rn5t568_wdt_probe,
	.of_compatible = DRV_OF_COMPAT(rn5t568_wdt_of_match),
};
device_platform_driver(rn5t568_wdt_driver);
