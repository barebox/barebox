// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <mach/at91/at91_ddrsdrc.h>
#include <mach/at91/barebox-arm.h>
#include <io.h>
#include <debug_ll.h>

extern char __dtb_z_at91sam9x5ek_start[];

AT91_ENTRY_FUNCTION(start_at91sam9x5ek, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();
	arm_setup_stack(AT91SAM9X5_SRAM_BASE + AT91SAM9X5_SRAM_SIZE);

	fdt = __dtb_z_at91sam9x5ek_start + get_runtime_offset();

	barebox_arm_entry(AT91_CHIPSELECT_1, at91sam9x5_get_ddram_size(), fdt);
}
