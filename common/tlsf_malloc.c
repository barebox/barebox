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

#include <linux/kasan.h>
#include <linux/list.h>

tlsf_t tlsf_mem_pool;

struct pool_entry {
	pool_t pool;
	struct list_head list;
};

static LIST_HEAD(mem_pool_list);

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
	struct pool_entry *cur_pool;
	struct malloc_stats s;

	s.used = 0;
	s.free = 0;

	list_for_each_entry(cur_pool, &mem_pool_list, list)
		tlsf_walk_pool(cur_pool->pool, malloc_walker, &s);

	printf("used: %zu\nfree: %zu\n", s.used, s.free);
}

void *malloc_add_pool(void *mem, size_t bytes)
{
	pool_t new_pool;
	struct pool_entry *new_pool_entry;

	if (!mem)
		return NULL;

	if (!tlsf_mem_pool) {
		tlsf_mem_pool = tlsf_create(mem);
		mem = (char *)mem + tlsf_size();
		bytes = bytes - tlsf_size();
	}

	new_pool = tlsf_add_pool(tlsf_mem_pool, mem, bytes);
	if (!new_pool)
		return NULL;

	kasan_poison_shadow(mem, bytes, KASAN_TAG_INVALID);

	new_pool_entry = malloc(sizeof(*new_pool_entry));
	if (!new_pool_entry)
		return NULL;

	new_pool_entry->pool = new_pool;
	list_add(&new_pool_entry->list, &mem_pool_list);

	return (void *)new_pool;
}
