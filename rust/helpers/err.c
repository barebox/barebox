// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/helpers/err.c?id=05f7e89ab9731565d8a62e3b5d1ec206485eeb0b

#include <linux/err.h>

__force void *rust_helper_ERR_PTR(long err)
{
	return ERR_PTR(err);
}

bool rust_helper_IS_ERR(__force const void *ptr)
{
	return IS_ERR(ptr);
}

long rust_helper_PTR_ERR(__force const void *ptr)
{
	return PTR_ERR(ptr);
}
