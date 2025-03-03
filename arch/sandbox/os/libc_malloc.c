// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <stdlib.h>
#include <malloc.h>

#define BAREBOX_ENOMEM 12
#define BAREBOX_MALLOC_MAX_SIZE 0x40000000

extern int barebox_errno;

void barebox_malloc_stats(void)
{
}

void *barebox_memalign(size_t alignment, size_t bytes)
{
	void *mem = NULL;

	if (alignment <= BAREBOX_MALLOC_MAX_SIZE && bytes <= BAREBOX_MALLOC_MAX_SIZE)
		mem = memalign(alignment, bytes);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}

void *barebox_malloc(size_t size)
{

	void *mem = NULL;

	if (size <= BAREBOX_MALLOC_MAX_SIZE)
		mem = malloc(size);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}

size_t barebox_malloc_usable_size(void *mem)
{
	return malloc_usable_size(mem);
}

void barebox_free(void *ptr)
{
	free(ptr);
}

void *barebox_realloc(void *ptr, size_t size)
{
	void *mem = NULL;

	if (size <= BAREBOX_MALLOC_MAX_SIZE)
		mem = realloc(ptr, size);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}

void *barebox_calloc(size_t n, size_t elem_size)
{
	size_t product;
	void *mem = NULL;

	if (!__builtin_mul_overflow(n, elem_size, &product) &&
	    product <= BAREBOX_MALLOC_MAX_SIZE)
		mem = calloc(n, elem_size);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}
