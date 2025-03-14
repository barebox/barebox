// SPDX-License-Identifier: GPL-2.0-only
/*
 * tlsf wrapper for barebox
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <malloc.h>
#include <string.h>

#include <stdio.h>
#include <module.h>
#include <tlsf.h>

extern tlsf_t tlsf_mem_pool;

void *malloc(size_t bytes)
{
	void *mem;

	mem = tlsf_malloc(tlsf_mem_pool, bytes);
	if (!mem)
		errno = ENOMEM;

	return mem;
}
EXPORT_SYMBOL(malloc);

void free(void *mem)
{
	tlsf_free(tlsf_mem_pool, mem);
}
EXPORT_SYMBOL(free);

size_t malloc_usable_size(void *mem)
{
	return tlsf_block_size(mem);
}
EXPORT_SYMBOL(malloc_usable_size);

void *realloc(void *oldmem, size_t bytes)
{
	void *mem = tlsf_realloc(tlsf_mem_pool, oldmem, bytes);
	if (!mem)
		errno = ENOMEM;

	return mem;
}
EXPORT_SYMBOL(realloc);

void *memalign(size_t alignment, size_t bytes)
{
	void *mem = tlsf_memalign(tlsf_mem_pool, alignment, bytes);
	if (!mem)
		errno = ENOMEM;

	return mem;
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

	tlsf_walk_pool(tlsf_get_pool(tlsf_mem_pool), malloc_walker, &s);

	printf("used: %zu\nfree: %zu\n", s.used, s.free);
}
