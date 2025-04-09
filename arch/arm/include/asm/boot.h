/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _ASM_BOOT_H_
#define _ASM_BOOT_H_

#include <linux/types.h>

#ifdef CONFIG_ARM64
static inline void jump_to_linux(const void *_kernel, phys_addr_t dtb)
{
	void (*kernel)(unsigned long dtb, unsigned long x1, unsigned long x2,
		       unsigned long x3) = _kernel;
	kernel(dtb, 0, 0, 0);
}
#else
static inline void __jump_to_linux(const void *_kernel, unsigned arch,
				   phys_addr_t params)
{
	void (*kernel)(int zero, unsigned arch, ulong params) = _kernel;
	kernel(0, arch, params);
}
static inline void jump_to_linux(const void *_kernel, phys_addr_t dtb)
{
	__jump_to_linux(_kernel, ~0, dtb);
}
#endif

#endif	/* _ASM_BOOT_H_ */
