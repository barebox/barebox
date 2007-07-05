
#include <common.h>
#include <malloc.h>

void *xmalloc(size_t size)
{
	void *p = NULL;

	if (!(p = malloc(size)))
		panic("ERROR: out of memory\n");

	return p;
}

void *xrealloc(void *ptr, size_t size)
{
	void *p = NULL;

	if (!(p = realloc(ptr, size)))
		panic("ERROR: out of memory\n");

	return p;
}
