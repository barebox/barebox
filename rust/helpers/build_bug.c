// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/helpers/build_bug.c?id=05f7e89ab9731565d8a62e3b5d1ec206485eeb0b

#include <errno.h>

/*
 * barebox has no errname(). strerror_r() returns a message instead of the
 * symbol name, or NULL for error numbers without one.
 */
const char *rust_helper_errname(int err)
{
	return strerror_r(err, NULL, 0);
}
