// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2020 Ahmad Fatoum, Pengutronix
 */

#define pr_fmt(fmt) "stm32-rng: " fmt

#include <common.h>
#include <clock.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/hw_random.h>
#include <linux/iopoll.h>
#include <linux/reset.h>

#define RNG_CR 0x00
#define RNG_CR_RNGEN BIT(2)
#define RNG_CR_CED BIT(5)

#define RNG_SR 0x04
#define RNG_SR_SEIS BIT(6)
#define RNG_SR_CEIS BIT(5)
#define RNG_SR_SECS BIT(2)
#define RNG_SR_DRDY BIT(0)

#define RNG_DR 0x08

struct stm32_rng {
	struct clk	*clk;
	void __iomem	*base;
	struct hwrng	hwrng;
};

static inline struct stm32_rng *to_stm32_rng(struct hwrng *hwrng)
{
	return container_of(hwrng, struct stm32_rng, hwrng);
}

static int stm32_rng_read(struct hwrng *hwrng, void *data, size_t len, bool wait)
{
	int ret = 0;
	u32 sr, count, reg;
	size_t increment;
	struct stm32_rng *rng = to_stm32_rng(hwrng);
	size_t remaining = len;

	while (remaining) {
		ret = readl_poll_timeout(rng->base + RNG_SR, sr,
					 sr & RNG_SR_DRDY, 10 * USEC_PER_MSEC);
		if (ret)
			return ret;

		if (sr & (RNG_SR_SEIS | RNG_SR_SECS)) {
			int i;
			/* As per SoC TRM */
			clrbits_le32(rng->base + RNG_SR, RNG_SR_SEIS);
			for (i = 0; i < 12; i++)
				readl(rng->base + RNG_DR);
			if (readl(rng->base + RNG_SR) & RNG_SR_SEIS) {
				pr_warn("RNG Noise");
				return -EIO;
			}

			/* start again */
			continue;
		}

		/*
		 * Once the DRDY bit is set, the RNG_DR register can
		 * be read four consecutive times.
		 */
		count = 4;
		while (remaining && count) {
			reg = readl(rng->base + RNG_DR);
			memcpy(data, &reg, min(remaining, sizeof(u32)));
			increment = min(remaining, sizeof(u32));
			data += increment;
			remaining -= increment;
			count--;
		}
	}

	return len;
}

static int stm32_rng_init(struct hwrng *hwrng)
{
	int err;
	struct stm32_rng *rng = to_stm32_rng(hwrng);

	err = clk_enable(rng->clk);
	if (err)
		return err;

	/* Disable CED */
	writel(RNG_CR_RNGEN | RNG_CR_CED, rng->base + RNG_CR);

	/* clear error indicators */
	writel(0, rng->base + RNG_SR);

	return 0;
}

static void stm32_rng_remove(struct device *dev)
{
	struct stm32_rng *rng = dev->priv;

	writel(0, rng->base + RNG_CR);
	clk_disable(rng->clk);
}

static int stm32_rng_probe(struct device *dev)
{
	struct stm32_rng *rng;
	struct resource *res;
	int ret;

	rng = xzalloc(sizeof(*rng));
	dev->priv = rng;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	rng->base = IOMEM(res->start);

	rng->clk = clk_get(dev, NULL);
	if (IS_ERR(rng->clk)) {
		dev_err(dev, "Can not get clock\n");
		return PTR_ERR(rng->clk);
	}

	ret = device_reset_us(dev, 20);
	if (ret)
		return ret;

	rng->hwrng.name = dev->name;
	rng->hwrng.read = stm32_rng_read;
	rng->hwrng.init = stm32_rng_init;

	ret = hwrng_register(dev, &rng->hwrng);
	if (ret)
		stm32_rng_remove(dev);

	return ret;
}

static const struct of_device_id stm32_rng_dt_ids[] = {
	{ .compatible = "st,stm32-rng" },
	{ /* sentinel */},
};
MODULE_DEVICE_TABLE(of, stm32_rng_dt_ids);

static struct driver stm32_rng_driver = {
	.name = "stm32-rng",
	.probe = stm32_rng_probe,
	.remove = stm32_rng_remove,
	.of_compatible = stm32_rng_dt_ids,
};
device_platform_driver(stm32_rng_driver);
