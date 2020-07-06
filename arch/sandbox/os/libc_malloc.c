// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <stdlib.h>
#include <malloc.h>

void barebox_malloc_stats(void)
{
}

void *barebox_memalign(size_t alignment, size_t bytes)
{
	return memalign(alignment, bytes);
}

void *barebox_malloc(size_t size)
{
	return malloc(size);
}

void barebox_free(void *ptr)
{
	free(ptr);
}

void *barebox_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void *barebox_calloc(size_t n, size_t elem_size)
{
	return calloc(n, elem_size);
}
