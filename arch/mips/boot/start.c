/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2011, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <linux/kernel.h>

void __noreturn _start(void *fdt, u32 fdt_size, u32 relocaddr);
void __noreturn relocate_code(void *fdt, u32 fdt_size, u32 relocaddr);

void __noreturn __section(.text_entry) _start(void *fdt, u32 fdt_size,
					      u32 relocaddr)
{
	relocate_code(fdt, fdt_size, relocaddr);
}
