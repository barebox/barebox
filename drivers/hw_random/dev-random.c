// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <linux/hw_random.h>
#include <mach/linux.h>

struct devrandom_rnd {
	struct hwrng hwrng;
	devrandom_t *priv;
};

static inline struct devrandom_rnd *to_rnd(struct hwrng *hwrng)
{
	return container_of(hwrng, struct devrandom_rnd, hwrng);
}

static int devrandom_rnd_read(struct hwrng *hwrng, void *buf, size_t max, bool wait)
{
	return devrandom_read(to_rnd(hwrng)->priv, buf, max, wait);
}

static int devrandom_rnd_init(struct hwrng *hwrng)
{
	devrandom_t *devrandom = devrandom_init();
	if (IS_ERR(devrandom))
		return PTR_ERR(devrandom);

	to_rnd(hwrng)->priv = devrandom;
	return 0;
}

static int devrandom_rnd_probe(struct device *dev)
{
	struct devrandom_rnd *rnd;
	int ret;

	rnd = xzalloc(sizeof(*rnd));

	rnd->hwrng.name = dev->name;
	rnd->hwrng.read = devrandom_rnd_read;
	rnd->hwrng.init = devrandom_rnd_init;

	ret = hwrng_register(dev, &rnd->hwrng);
	if (ret) {
		dev_err(dev, "failed to register: %pe\n", ERR_PTR(ret));

		return ret;
	}

	dev_info(dev, "Registered.\n");

	return 0;
}

static struct driver devrandom_rnd_driver = {
	.name	= "devrandom",
	.probe	= devrandom_rnd_probe,
};
device_platform_driver(devrandom_rnd_driver);
