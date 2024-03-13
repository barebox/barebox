// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2011 Peter Korsgaard <jacmet@sunsite.dk>

#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/hw_random.h>
#include <of.h>
#include <linux/device.h>

#define TRNG_CR		0x00
#define TRNG_MR		0x04
#define TRNG_ISR	0x1c
#define TRNG_ISR_DATRDY	BIT(0)
#define TRNG_ODATA	0x50

#define TRNG_KEY	0x524e4700 /* RNG */

#define TRNG_HALFR	BIT(0) /* generate RN every 168 cycles */

struct atmel_trng_data {
	bool has_half_rate;
};

struct atmel_trng {
	struct clk *clk;
	void __iomem *base;
	struct hwrng rng;
	bool has_half_rate;
};

static bool atmel_trng_wait_ready(struct atmel_trng *trng, bool wait)
{
	int ready;

	ready = readl(trng->base + TRNG_ISR) & TRNG_ISR_DATRDY;
	if (!ready && wait)
		readl_poll_timeout(trng->base + TRNG_ISR, ready,
				   ready & TRNG_ISR_DATRDY, 20000);

	return !!ready;
}

static int atmel_trng_read(struct hwrng *rng, void *buf, size_t max,
			   bool wait)
{
	struct atmel_trng *trng = container_of(rng, struct atmel_trng, rng);
	u32 *data = buf;
	int ret;

	ret = atmel_trng_wait_ready(trng, wait);
	if (!ret)
		return 0;

	*data = readl(trng->base + TRNG_ODATA);
	/*
	 * ensure data ready is only set again AFTER the next data word is ready
	 * in case it got set between checking ISR and reading ODATA, so we
	 * don't risk re-reading the same word
	 */
	readl(trng->base + TRNG_ISR);
	ret = 4;

	return ret;
}

static int atmel_trng_init(struct hwrng *rng)
{
	struct atmel_trng *trng = container_of(rng, struct atmel_trng, rng);
	unsigned long rate;
	int ret;

	ret = clk_prepare_enable(trng->clk);
	if (ret)
		return ret;

	if (trng->has_half_rate) {
		rate = clk_get_rate(trng->clk);

		/* if peripheral clk is above 100MHz, set HALFR */
		if (rate > 100000000)
			writel(TRNG_HALFR, trng->base + TRNG_MR);
	}

	writel(TRNG_KEY | 1, trng->base + TRNG_CR);

	return 0;
}

static void atmel_trng_cleanup(struct atmel_trng *trng)
{
	writel(TRNG_KEY, trng->base + TRNG_CR);
	clk_disable_unprepare(trng->clk);
}

static int atmel_trng_probe(struct device *dev)
{
	struct atmel_trng *trng;
	const struct atmel_trng_data *data;

	trng = devm_kzalloc(dev, sizeof(*trng), GFP_KERNEL);
	if (!trng)
		return -ENOMEM;

	trng->base = dev_platform_ioremap_resource(dev, 0);
	if (IS_ERR(trng->base))
		return PTR_ERR(trng->base);

	trng->clk = clk_get(dev, NULL);
	if (IS_ERR(trng->clk))
		return PTR_ERR(trng->clk);
	data = device_get_match_data(dev);
	if (!data)
		return -ENODEV;

	trng->has_half_rate = data->has_half_rate;
	trng->rng.name = dev_name(dev);
	trng->rng.read = atmel_trng_read;
	trng->rng.init = atmel_trng_init;
	dev->priv = trng;

	return hwrng_register(dev, &trng->rng);
}

static void atmel_trng_remove(struct device *dev)
{
	atmel_trng_cleanup(dev->priv);
}

static const struct atmel_trng_data at91sam9g45_config = {
	.has_half_rate = false,
};

static const struct atmel_trng_data sam9x60_config = {
	.has_half_rate = true,
};

static const struct of_device_id atmel_trng_dt_ids[] = {
	{
		.compatible = "atmel,at91sam9g45-trng",
		.data = &at91sam9g45_config,
	}, {
		.compatible = "microchip,sam9x60-trng",
		.data = &sam9x60_config,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, atmel_trng_dt_ids);

static struct driver atmel_trng_driver = {
	.name		= "atmel-trng",
	.probe		= atmel_trng_probe,
	.remove		= atmel_trng_remove,
	.of_match_table = atmel_trng_dt_ids,
};
device_platform_driver(atmel_trng_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Korsgaard <jacmet@sunsite.dk>");
MODULE_DESCRIPTION("Atmel true random number generator driver");
