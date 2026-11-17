// SPDX-License-Identifier: GPL-2.0-or-later

#include <malloc.h>
#include <string.h>
#include <linux/minmax.h>

void free_sensitive(void *mem)
{
	size_t size = malloc_usable_size(mem);

	if (size)
		memzero_explicit(mem, size);

	free(mem);
}

void *aligned_realloc(void *mem, size_t alignment, size_t size)
{
	void *newmem;

	if (alignment <= CONFIG_MALLOC_ALIGNMENT)
		return realloc(mem, size);

	/* allocate first, so mem stays valid on failure */
	newmem = memalign(alignment, size);
	if (!newmem || !mem)
		return newmem;

	memcpy(newmem, mem, min(malloc_usable_size(mem), size));
	free(mem);

	return newmem;
}
