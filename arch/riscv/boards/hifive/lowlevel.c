// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-riscv.h>
#include <debug_ll.h>

ENTRY_FUNCTION(start_hifive_unmatched, a0, a1, a2)
{
	extern char __dtb_z_hifive_unmatched_a00_start[];

	putc_ll('>');

	barebox_riscv_entry(0x80000000, SZ_128M,
			    __dtb_z_hifive_unmatched_a00_start + get_runtime_offset());
}

ENTRY_FUNCTION(start_hifive_unleashed, a0, a1, a2)
{
	extern char __dtb_z_hifive_unleashed_a00_start[];

	putc_ll('>');

	barebox_riscv_entry(0x80000000, SZ_128M,
			    __dtb_z_hifive_unleashed_a00_start + get_runtime_offset());
}
