// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-riscv.h>
#include <debug_ll.h>
#include <asm/riscv_nmon.h>

ENTRY_FUNCTION(start_litex_linux, a0, a1, a2)
{
	extern char __dtb_z_litex_linux_start[];
	void *fdt;

	barebox_nmon_entry();

	putc_ll('>');

	/* On POR, we are running from read-only memory here. */

	fdt = __dtb_z_litex_linux_start + get_runtime_offset();

	barebox_riscv_machine_entry(0x40000000, SZ_256M, fdt);
}
