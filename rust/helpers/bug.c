// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/helpers/bug.c?id=05f7e89ab9731565d8a62e3b5d1ec206485eeb0b

#include <linux/bug.h>

__noreturn void rust_helper_BUG(void)
{
	BUG();
}

bool rust_helper_WARN_ON(bool cond)
{
	return WARN_ON(cond);
}

/* __WARN() with the Rust caller's location */
void rust_helper_warn_at(const char *file, unsigned int line)
{
	__bug_printf("WARNING: at %s:%u!\n", file, line);
}
