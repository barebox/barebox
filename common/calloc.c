#include <common.h>
#include <malloc.h>

/*
 * calloc calls malloc, then zeroes out the allocated chunk.
 */
void *calloc(size_t n, size_t elem_size)
{
	size_t size = elem_size * n;
	void *r = malloc(size);

	if (!r)
		return r;

	memset(r, 0x0, size);

	return r;
}
EXPORT_SYMBOL(calloc);
