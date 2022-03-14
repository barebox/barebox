// SPDX-License-Identifier: GPL-2.0+

#include <mach/entry.h>
#include <debug_ll.h>

extern char __dtb_z_stm32mp135f_dk_start[];

ENTRY_FUNCTION(start_stm32mp13xx_dk, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	putc_ll('>');

	fdt = __dtb_z_stm32mp135f_dk_start  + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}
