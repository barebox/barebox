// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <malloc.h>
#include <memory.h>
#include <linux/overflow.h>

/*
 * calloc calls malloc, then zeroes out the allocated chunk.
 */
void *calloc(size_t n, size_t elem_size)
{
	size_t size = size_mul(elem_size, n);
	void *r = malloc(size);

	if (!ZERO_OR_NULL_PTR(r) && !want_init_on_alloc())
		memset(r, 0x0, size);

	return r;
}
EXPORT_SYMBOL(calloc);
