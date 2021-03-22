// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-riscv.h>
#include <debug_ll.h>

ENTRY_FUNCTION(start_erizo_generic, a0, a1, a2)
{
	extern char __dtb_z_erizo_generic_start[];

	debug_ll_ns16550_init();
	putc_ll('>');

	/* On POR, we are running from read-only memory here. */

	barebox_riscv_entry(0x80000000, SZ_8M,
			    __dtb_z_erizo_generic_start + get_runtime_offset());
}
