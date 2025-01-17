// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 David Jander, Protonic Holland

#include <common.h>
#include <debug_ll.h>
#include <mach/stm32mp/entry.h>

extern char __dtb_z_stm32mp133c_prihmb_start[];

ENTRY_FUNCTION(start_stm32mp133c_prihmb, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	/* first stage has set up the UART, so nothing to do here */
	putc_ll('>');

	fdt = __dtb_z_stm32mp133c_prihmb_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}
