// SPDX-License-Identifier: GPL-2.0-only
#include <pbl.h>

static unsigned long free_mem_ptr;
static unsigned long free_mem_end_ptr;
static unsigned long malloc_ptr;
static int malloc_count;

void *pbl_malloc(int size)
{
	void *p;

	if (size < 0)
		return NULL;
	if (!malloc_ptr)
		malloc_ptr = free_mem_ptr;

	malloc_ptr = (malloc_ptr + 3) & ~3;     /* Align */

	p = (void *)malloc_ptr;
	malloc_ptr += size;

	if (free_mem_end_ptr && malloc_ptr >= free_mem_end_ptr)
		return NULL;

	malloc_count++;
	return p;
}

void pbl_free(void *where)
{
	malloc_count--;
	if (!malloc_count)
		malloc_ptr = free_mem_ptr;
}

void pbl_malloc_init(unsigned long base, size_t size)
{
	free_mem_ptr = base;
	malloc_ptr = base;
	malloc_count = 0;
	free_mem_end_ptr = base + size;
}