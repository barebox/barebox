// SPDX-License-Identifier: GPL-2.0

#include <malloc.h>

void rust_helper_free(const void *ptr)
{
	return free((void *)ptr);
}
