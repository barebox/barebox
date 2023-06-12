// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 */

#include <common.h>
#include <init.h>
#include <watchdog.h>
#include <asm/io.h>
#include <of_device.h>
#include <linux/log2.h>
#include <linux/iopoll.h>
#include <linux/clk.h>

/* IWDG registers */
#define IWDG_KR		0x00	/* Key register */
#define IWDG_PR		0x04	/* Prescaler Register */
#define IWDG_RLR	0x08	/* ReLoad Register */
#define IWDG_SR		0x0C	/* Status Register */

/* IWDG_KR register bit mask */
#define KR_KEY_RELOAD	0xAAAA	/* Reload counter enable */
#define KR_KEY_ENABLE	0xCCCC	/* Peripheral enable */
#define KR_KEY_EWA	0x5555	/* Write access enable */

/* IWDG_PR register bit values */
#define PR_SHIFT	2

/* IWDG_RLR register values */
#define RLR_MAX		GENMASK(11, 0)

/* IWDG_SR register bit mask */
#define SR_PVU	BIT(0) /* Watchdog prescaler value update */
#define SR_RVU	BIT(1) /* Watchdog counter reload value update */

/* set timeout to 100 ms */
#define TIMEOUT_US	100000

struct stm32_iwdg {
	struct watchdog wdd;
	void __iomem *iwdg_base;
	unsigned int timeout;
	unsigned int rate;
};

static inline struct stm32_iwdg *to_stm32_iwdg(struct watchdog *wdd)
{
	return container_of(wdd, struct stm32_iwdg, wdd);
}

static void stm32_iwdg_ping(struct stm32_iwdg *wd)
{
	writel(KR_KEY_RELOAD, wd->iwdg_base + IWDG_KR);
}

static int stm32_iwdg_start(struct stm32_iwdg *wd, unsigned int timeout)
{
	u32 presc, iwdg_rlr, iwdg_pr, iwdg_sr;
	int ret;

	presc = DIV_ROUND_UP(timeout * wd->rate, RLR_MAX + 1);

	/* The prescaler is align on power of 2 and start at 2 ^ PR_SHIFT. */
	presc = roundup_pow_of_two(presc);
	iwdg_pr = presc <= 1 << PR_SHIFT ? 0 : ilog2(presc) - PR_SHIFT;
	iwdg_rlr = ((timeout * wd->rate) / presc) - 1;

	/* enable write access */
	writel(KR_KEY_EWA, wd->iwdg_base + IWDG_KR);

	/* set prescaler & reload registers */
	writel(iwdg_pr, wd->iwdg_base + IWDG_PR);
	writel(iwdg_rlr, wd->iwdg_base + IWDG_RLR);
	writel(KR_KEY_ENABLE, wd->iwdg_base + IWDG_KR);

	/* wait for the registers to be updated (max 100ms) */
	ret = readl_poll_timeout(wd->iwdg_base + IWDG_SR, iwdg_sr,
				 !(iwdg_sr & (SR_PVU | SR_RVU)),
				 TIMEOUT_US);
	if (!ret)
		wd->timeout = timeout;

	return ret;
}


static int stm32_iwdg_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct stm32_iwdg *wd = to_stm32_iwdg(wdd);
	int ret;

	if (!timeout)
		return -ENOSYS; /* can't disable */

	if (timeout > wdd->timeout_max)
		return -EINVAL;

	if (wd->timeout != timeout) {
		ret = stm32_iwdg_start(wd, timeout);
		if (ret) {
			dev_err(wdd->hwdev, "Fail to (re)start watchdog\n");
			return ret;
		}
	}

	stm32_iwdg_ping(wd);
	return 0;
}

struct stm32_iwdg_data {
	bool has_pclk;
	u32 max_prescaler;
};

static const struct stm32_iwdg_data stm32_iwdg_data = {
	.has_pclk = false, .max_prescaler = 256,
};

static const struct stm32_iwdg_data stm32mp1_iwdg_data = {
	.has_pclk = true, .max_prescaler = 1024,
};

static const struct of_device_id stm32_iwdg_of_match[] = {
	{ .compatible = "st,stm32-iwdg",    .data = &stm32_iwdg_data },
	{ .compatible = "st,stm32mp1-iwdg", .data = &stm32mp1_iwdg_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stm32_iwdg_of_match);

static int stm32_iwdg_probe(struct device *dev)
{
	struct stm32_iwdg_data *data;
	struct stm32_iwdg *wd;
	struct resource *res;
	struct watchdog *wdd;
	struct clk *clk;
	int ret;

	wd = xzalloc(sizeof(*wd));

	ret = dev_get_drvdata(dev, (const void **)&data);
	if (ret)
		return -ENODEV;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res)) {
		dev_err(dev, "could not get timer memory region\n");
		return PTR_ERR(res);
	}
	wd->iwdg_base = IOMEM(res->start);

	clk = clk_get(dev, "lsi");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	ret = clk_enable(clk);
	if (ret)
		return ret;

	wd->rate = clk_get_rate(clk);
	if (wd->rate == 0)
		return -EINVAL;

	if (data->has_pclk) {
		clk = clk_get(dev, "pclk");
		if (IS_ERR(clk))
			return PTR_ERR(clk);

		ret = clk_enable(clk);
		if (ret)
			return ret;
	}

	wdd = &wd->wdd;
	wdd->hwdev = dev;
	wdd->set_timeout = stm32_iwdg_set_timeout;
	wdd->timeout_max = (RLR_MAX + 1) * data->max_prescaler * 1000;
	wdd->timeout_max /= wd->rate * 1000;
	wdd->running = WDOG_HW_RUNNING_UNSUPPORTED; /* ONF bit not present in IP */

	ret = watchdog_register(wdd);
	if (ret) {
		dev_err(dev, "Failed to register watchdog device\n");
		return ret;
	}

	dev_info(dev, "probed\n");
	return 0;
}

static struct driver stm32_iwdg_driver = {
	.name  = "stm32-iwdg",
	.probe = stm32_iwdg_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_iwdg_of_match),
};
device_platform_driver(stm32_iwdg_driver);
