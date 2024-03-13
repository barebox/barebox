// SPDX-License-Identifier: GPL-2.0-only
/*
 * drivers/char/hw_random/timeriomem-rng.c
 *
 * Copyright (C) 2009 Alexander Clouter <alex@digriz.org.uk>
 *
 * Derived from drivers/char/hw_random/omap-rng.c
 *   Copyright 2005 (c) MontaVista Software, Inc.
 *   Author: Deepak Saxena <dsaxena@plexity.net>
 *
 * Overview:
 *   This driver is useful for platforms that have an IO range that provides
 *   periodic random data from a single IO memory address.  All the platform
 *   has to do is provide the address and 'wait time' that new data becomes
 *   available.
 *
 * TODO: add support for reading sizes other than 32bits and masking
 */

#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <of.h>
#include <linux/device.h>
#include <linux/time.h>

struct timeriomem_rng_private {
	void __iomem		*io_base;

	ktime_t			period;
	ktime_t			next_read;

	struct hwrng		rng_ops;
};

static int timeriomem_rng_read(struct hwrng *hwrng, void *data,
				size_t max, bool wait)
{
	struct timeriomem_rng_private *priv =
		container_of(hwrng, struct timeriomem_rng_private, rng_ops);
	int retval = 0;
	int period_us = ktime_to_us(priv->period);
	ktime_t now = ktime_get();

	/*
	 * There may not have been enough time for new data to be generated
	 * since the last request.  If the caller doesn't want to wait, let them
	 * bail out.  Otherwise, wait for the completion.  If the new data has
	 * already been generated, the completion should already be available.
	 */
	if (ktime_before(now, priv->next_read)) {
		if (!wait)
			return 0;

		udelay(ktime_to_us(ktime_sub(priv->next_read, now)));
	}

	do {
		/*
		 * After the first read, all additional reads will need to wait
		 * for the RNG to generate new data.  Since the period can have
		 * a wide range of values (1us to 1s have been observed), allow
		 * for 1% tolerance in the sleep time rather than a fixed value.
		 */
		if (retval > 0)
			udelay(period_us);

		*(u32 *)data = readl(priv->io_base);
		retval += sizeof(u32);
		data += sizeof(u32);
		max -= sizeof(u32);
	} while (wait && max > sizeof(u32));

	/*
	 * Block any new callers until the RNG has had time to generate new
	 * data.
	 */
	priv->next_read = ktime_add(ktime_get(), priv->period);

	return retval;
}

static int timeriomem_rng_probe(struct device *dev)
{
	struct timeriomem_rng_private *priv;
	struct resource *res;
	int err = 0;
	int period;

	/* Allocate memory for the device structure (and zero it) */
	priv = devm_kzalloc(dev,
			sizeof(struct timeriomem_rng_private), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->io_base = dev_platform_get_and_ioremap_resource(dev, 0, &res);
	if (IS_ERR(priv->io_base))
		return PTR_ERR(priv->io_base);

	if (res->start % 4 != 0 || resource_size(res) < 4) {
		dev_err(dev,
			"address must be at least four bytes wide and 32-bit aligned\n");
		return -EINVAL;
	}

	if (of_property_read_u32(dev->of_node, "period", &period))
		return dev_err_probe(dev, -EINVAL, "missing period\n");

	priv->period = ns_to_ktime(period * NSEC_PER_USEC);

	priv->rng_ops.name = dev_name(dev);
	priv->rng_ops.read = timeriomem_rng_read;

	/* Assume random data is already available. */
	priv->next_read = ktime_get();

	err = hwrng_register(dev, &priv->rng_ops);
	if (err) {
		dev_err(dev, "problem registering\n");
		return err;
	}

	dev_info(dev, "32bits from 0x%p @ %dus\n",
			priv->io_base, period);

	return 0;
}

static const struct of_device_id timeriomem_rng_match[] = {
	{ .compatible = "timeriomem_rng" },
	{},
};
MODULE_DEVICE_TABLE(of, timeriomem_rng_match);

static struct driver timeriomem_rng_driver = {
	.name		= "timeriomem_rng",
	.of_match_table	= timeriomem_rng_match,
	.probe		= timeriomem_rng_probe,
};

device_platform_driver(timeriomem_rng_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexander Clouter <alex@digriz.org.uk>");
MODULE_DESCRIPTION("Timer IOMEM H/W RNG driver");
