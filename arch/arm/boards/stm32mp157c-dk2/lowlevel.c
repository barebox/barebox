// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <mach/entry.h>
#include <debug_ll.h>

extern char __dtb_z_stm32mp157c_dk2_start[];

static void setup_uart(void)
{
	/* first stage has set up the UART, so nothing to do here */
	putc_ll('>');
}

ENTRY_FUNCTION(start_stm32mp157c_dk2, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp157c_dk2_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}
