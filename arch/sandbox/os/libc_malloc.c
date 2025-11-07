// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <stdlib.h>
#include <malloc.h>

#define ZERO_SIZE_PTR ((void *)16)

#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= \
				(unsigned long)ZERO_SIZE_PTR)
#define BAREBOX_ENOMEM 12
#define BAREBOX_MALLOC_MAX_SIZE 0x40000000

extern int barebox_errno;

void barebox_malloc_stats(void)
{
}

void *barebox_memalign(size_t alignment, size_t bytes)
{
	void *mem = NULL;

	if (!bytes)
		return ZERO_SIZE_PTR;

	if (alignment <= BAREBOX_MALLOC_MAX_SIZE && bytes <= BAREBOX_MALLOC_MAX_SIZE)
		mem = memalign(alignment, bytes);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}

void *barebox_malloc(size_t size)
{
	void *mem = NULL;

	if (!size)
		return ZERO_SIZE_PTR;

	if (size <= BAREBOX_MALLOC_MAX_SIZE)
		mem = malloc(size);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}

size_t barebox_malloc_usable_size(void *mem)
{
	if (ZERO_OR_NULL_PTR(mem))
		return 0;
	return malloc_usable_size(mem);
}

void barebox_free(void *ptr)
{
	if (ZERO_OR_NULL_PTR(ptr))
		return;
	free(ptr);
}

void *barebox_realloc(void *ptr, size_t size)
{
	void *mem = NULL;

	if (!size) {
		barebox_free(ptr);
		return ZERO_SIZE_PTR;
	}
	if (ZERO_OR_NULL_PTR(ptr))
		ptr = NULL;

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

	if (!n || !elem_size)
		return ZERO_SIZE_PTR;

	if (!__builtin_mul_overflow(n, elem_size, &product) &&
	    product <= BAREBOX_MALLOC_MAX_SIZE)
		mem = calloc(n, elem_size);
	if (!mem)
		barebox_errno = BAREBOX_ENOMEM;

	return mem;
}

#ifdef CONFIG_DEBUG_MEMLEAK
void barebox_memleak_check(void)
{
	void __lsan_do_recoverable_leak_check(void);

	__lsan_do_recoverable_leak_check();
}
#endif
