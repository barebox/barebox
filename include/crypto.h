/*
 * Copyright (C) 2016 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __CRYPTO_H__
#define __CRYPTO_H__

struct ablkcipher_request {
	unsigned int nbytes;

	void __iomem *info;

	void __iomem *dst;
	void __iomem *src;
};

noinline unsigned long __crypto_memneq(const void *a, const void *b, size_t size);

/**
 * crypto_memneq - Compare two areas of memory without leaking
 *                 timing information.
 *
 * @a: One area of memory
 * @b: Another area of memory
 * @size: The size of the area.
 *
 * Returns 0 when data is equal, 1 otherwise.
 */
static inline int crypto_memneq(const void *a, const void *b, size_t size)
{
	return __crypto_memneq(a, b, size) != 0UL ? 1 : 0;
}

#endif
