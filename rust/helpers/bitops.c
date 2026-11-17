// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/helpers/bitops.c?id=7d0a66e4bb9081d75c82ec4957c50034cb0ea449

#include <linux/bitops.h>

void rust_helper___set_bit(unsigned long nr, unsigned long *addr)
{
	__set_bit(nr, addr);
}

void rust_helper___clear_bit(unsigned long nr, unsigned long *addr)
{
	__clear_bit(nr, addr);
}

void rust_helper_set_bit(unsigned long nr, volatile unsigned long *addr)
{
	set_bit(nr, addr);
}

void rust_helper_clear_bit(unsigned long nr, volatile unsigned long *addr)
{
	clear_bit(nr, addr);
}

unsigned long rust_helper_find_next_bit(const unsigned long *addr,
					unsigned long size,
					unsigned long offset)
{
	return find_next_bit(addr, size, offset);
}

unsigned long rust_helper_find_next_zero_bit(const unsigned long *addr,
					     unsigned long size,
					     unsigned long offset)
{
	return find_next_zero_bit(addr, size, offset);
}
