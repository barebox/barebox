// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 */
#include <common.h>
#include <malloc.h>

void malloc_stats(void)
{
}

void *memalign(size_t alignment, size_t bytes)
{
	void *mem = NULL;

	if (alignment <= MALLOC_MAX_SIZE && bytes <= MALLOC_MAX_SIZE)
		mem = sbrk(bytes + alignment);
	if (!mem) {
		errno = ENOMEM;
		return NULL;
	}

	return PTR_ALIGN(mem, alignment);
}

void *malloc(size_t size)
{
	return memalign(CONFIG_MALLOC_ALIGNMENT, size);
}

void free(void *ptr)
{
}

size_t malloc_usable_size(void *mem)
{
	BUG();
}

void *realloc(void *ptr, size_t size)
{
	BUG();
}
