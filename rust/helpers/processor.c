// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/helpers/processor.c?id=05f7e89ab9731565d8a62e3b5d1ec206485eeb0b

#include <linux/processor.h>

void rust_helper_cpu_relax(void)
{
	cpu_relax();
}
