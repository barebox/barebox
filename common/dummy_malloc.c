/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * As a special exception, if other files instantiate templates or use macros
 * or inline functions from this file, or you compile this file and link it
 * with other works to produce a work based on this file, this file does not
 * by itself cause the resulting work to be covered by the GNU General Public
 * License. However the source code for this file must still be made available
 * in accordance with section (3) of the GNU General Public License.

 * This exception does not invalidate any other reasons why a work based on
 * this file might be covered by the GNU General Public License.
 */
#include <common.h>
#include <malloc.h>

void malloc_stats(void)
{
}

void *memalign(size_t alignment, size_t bytes)
{
	unsigned long mem = (unsigned long)sbrk(bytes + alignment);

	mem = (mem + alignment) & ~(alignment - 1);

	return (void *)mem;
}

void *malloc(size_t size)
{
	return memalign(8, size);
}

void free(void *ptr)
{
}

void *realloc(void *ptr, size_t size)
{
	BUG();
}

void *calloc(size_t n, size_t elem_size)
{
	size_t size = elem_size * n;
	void *r = malloc(size);

	if (!r)
		return r;

	memset(r, 0x0, size);

	return r;
}
