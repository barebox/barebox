#include <common.h>
#include <asm/system.h>
#include <string.h>

void *__arch_memset(void *dst, int c, __kernel_size_t size);
void *__arch_memcpy(void * dest, const void *src, size_t count);

void *memset(void *dst, int c, __kernel_size_t size)
{
	if (likely(get_cr() & CR_M))
		return __arch_memset(dst, c, size);

	return __default_memset(dst, c, size);
}

void *memcpy(void * dest, const void *src, size_t count)
{
	if (likely(get_cr() & CR_M))
		return __arch_memcpy(dest, src, count);

	return __default_memcpy(dest, src, count);
}