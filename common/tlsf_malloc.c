/*
 * tlsf wrapper for barebox
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <malloc.h>
#include <string.h>

#include <stdio.h>
#include <module.h>
#include <tlsf.h>

extern tlsf_pool tlsf_mem_pool;

void *malloc(size_t bytes)
{
	/*
	 * tlsf_malloc returns NULL for zero bytes, we instead want
	 * to have a valid pointer.
	 */
	if (!bytes)
		bytes = 1;

	return tlsf_malloc(tlsf_mem_pool, bytes);
}
EXPORT_SYMBOL(malloc);

/*
 * calloc calls malloc, then zeroes out the allocated chunk.
 */
void *calloc(size_t n, size_t elem_size)
{
	void *mem;
	size_t sz;

	sz = n * elem_size;
	mem = malloc(sz);
	memset(mem, 0, sz);

	return mem;
}
EXPORT_SYMBOL(calloc);

void free(void *mem)
{
	tlsf_free(tlsf_mem_pool, mem);
}
EXPORT_SYMBOL(free);

void *realloc(void *oldmem, size_t bytes)
{
	return tlsf_realloc(tlsf_mem_pool, oldmem, bytes);
}
EXPORT_SYMBOL(realloc);

void *memalign(size_t alignment, size_t bytes)
{
	return tlsf_memalign(tlsf_mem_pool, alignment, bytes);
}
EXPORT_SYMBOL(memalign);

struct malloc_stats {
	size_t free;
	size_t used;
};

static void malloc_walker(void* ptr, size_t size, int used, void *user)
{
	struct malloc_stats *s = user;

	if (used)
		s->used += size;
	else
		s->free += size;
}

void malloc_stats(void)
{
	struct malloc_stats s;

	s.used = 0;
	s.free = 0;

	tlsf_walk_heap(tlsf_mem_pool, malloc_walker, &s);

	printf("used: %10d\nfree: %10d\n", s.used, s.free);
}
