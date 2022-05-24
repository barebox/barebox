// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru>

#include <common.h>
#include <asm/barebox-arm.h>
#include <linux/sizes.h>
#include <mach/clps711x.h>

extern char __dtb_ep7212_clep7212_start[];

ENTRY_FUNCTION_WITHSTACK(start_ep7212_clep7212,
			 CS6_BASE + 48 * SZ_1K, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_ep7212_clep7212_start;

	clps711x_start(fdt + get_runtime_offset());
}
