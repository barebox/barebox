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

