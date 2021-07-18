// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-riscv.h>
#include <debug_ll.h>

static __always_inline void start_hifive(unsigned long hartid, void *fdt)
{
	putc_ll('>');

	barebox_riscv_supervisor_entry(0x80000000, SZ_128M, hartid, fdt);
}

ENTRY_FUNCTION(start_hifive_unmatched, a0, a1, a2)
{
	extern char __dtb_z_hifive_unmatched_a00_start[];

	start_hifive(a0, __dtb_z_hifive_unmatched_a00_start + get_runtime_offset());
}

ENTRY_FUNCTION(start_hifive_unleashed, a0, a1, a2)
{
	extern char __dtb_z_hifive_unleashed_a00_start[];

	start_hifive(a0, __dtb_z_hifive_unleashed_a00_start + get_runtime_offset());
}
