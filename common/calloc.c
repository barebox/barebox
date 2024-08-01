// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <malloc.h>
#include <linux/overflow.h>

/*
 * calloc calls malloc, then zeroes out the allocated chunk.
 */
void *calloc(size_t n, size_t elem_size)
{
	size_t size = size_mul(elem_size, n);
	void *r = malloc(size);

	if (!r)
		return r;

	memset(r, 0x0, size);

	return r;
}
EXPORT_SYMBOL(calloc);
