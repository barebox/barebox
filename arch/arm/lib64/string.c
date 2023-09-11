// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/system.h>
#include <string.h>

void *__arch_memset(void *dst, int c, __kernel_size_t size);
void *__arch_memcpy(void * dest, const void *src, size_t count);

static __prereloc void *_memset(void *dst, int c, __kernel_size_t size)
{
	if (likely(get_cr() & CR_M))
		return __arch_memset(dst, c, size);

	return __nokasan_default_memset(dst, c, size);
}

void __weak *memset(void *dst, int c, __kernel_size_t size)
{
	return _memset(dst, c, size);
}

void *__memset(void *dst, int c, __kernel_size_t size)
	__alias(_memset);

static void *_memcpy(void * dest, const void *src, size_t count)
{
	if (likely(get_cr() & CR_M))
		return __arch_memcpy(dest, src, count);

	return __nokasan_default_memcpy(dest, src, count);
}

void __weak *memcpy(void * dest, const void *src, size_t count)
{
	return _memcpy(dest, src, count);
}

void *__memcpy(void * dest, const void *src, size_t count)
	__alias(_memcpy);
