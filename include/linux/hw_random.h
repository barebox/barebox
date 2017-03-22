/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef LINUX_HWRANDOM_H_
#define LINUX_HWRANDOM_H_

#include <linux/list.h>

/**
 * struct hwrng - Hardware Random Number Generator driver
 * @name:		Unique RNG name.
 * @init:		Initialization callback (can be NULL).
 * @read:		New API. drivers can fill up to max bytes of data
 *			into the buffer. The buffer is aligned for any type.
 */
struct hwrng {
	const char *name;
	int (*init)(struct hwrng *rng);
	int (*read)(struct hwrng *rng, void *data, size_t max, bool wait);

	struct list_head list;

	struct cdev cdev;
	struct device_d *dev;
	void *buf;
};

/* Register a new Hardware Random Number Generator driver. */
int hwrng_register(struct device_d *dev, struct hwrng *rng);
int hwrng_get_data(struct hwrng *rng, void *buffer, size_t size, int wait);

#ifdef CONFIG_HWRNG
struct hwrng *hwrng_get_first(void);
#else
static inline struct hwrng *hwrng_get_first(void) { return ERR_PTR(-ENODEV); };
#endif

#endif /* LINUX_HWRANDOM_H_ */
