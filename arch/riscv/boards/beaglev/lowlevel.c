// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <debug_ll.h>
#include <asm/barebox-riscv.h>
#include <asm/riscv_nmon.h>

ENTRY_FUNCTION(start_beaglev_starlight, a0, a1, a2)
{
	extern char __dtb_z_jh7100_beaglev_starlight_start[];
	void *fdt;

	debug_ll_init();
	barebox_nmon_entry();
	putc_ll('>');

	fdt = __dtb_z_jh7100_beaglev_starlight_start + get_runtime_offset();

	barebox_riscv_supervisor_entry(0x84000000, SZ_512M, a0, fdt);
}
