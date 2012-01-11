#include <common.h>
#include <malloc.h>

void *memalign(size_t alignment, size_t bytes)
{
	unsigned long mem = (unsigned long)sbrk(bytes + alignment);

	mem = (mem + alignment) & ~(alignment - 1);

	return (void *)mem;
}

void *malloc(size_t size)
{
	return memalign(8, size);
}

void free(void *ptr)
{
}

void *realloc(void *ptr, size_t size)
{
	BUG();
}

void *calloc(size_t n, size_t elem_size)
{
	size_t size = elem_size * n;
	void *r = malloc(size);

	if (!r)
		return r;

	memset(r, 0x0, size);

	return r;
}
