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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <malloc.h>
#include <string.h>
#include <malloc.h>

#include <stdio.h>
#include <module.h>
#include <tlsf.h>

extern tlsf_pool tlsf_mem_pool;

void *malloc(size_t bytes)
{
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

#ifdef CONFIG_CMD_MEMINFO
void malloc_stats(void)
{
}
#endif /* CONFIG_CMD_MEMINFO */
