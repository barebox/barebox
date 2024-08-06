// SPDX-License-Identifier: GPL-2.0-or-later

#include <malloc.h>
#include <string.h>

void free_sensitive(void *mem)
{
	size_t size = malloc_usable_size(mem);

	if (size)
		memzero_explicit(mem, size);

	free(mem);
}
