// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <mach/entry.h>
#include <debug_ll.h>
#include <mach/revision.h>

extern char __dtb_z_stm32mp157c_dk2_start[];
extern char __dtb_z_stm32mp157a_dk1_start[];

static void setup_uart(void)
{
	/* first stage has set up the UART, so nothing to do here */
	putc_ll('>');
}

ENTRY_FUNCTION(start_stm32mp15xx_dkx, r0, r1, r2)
{
	void *fdt;
	u32 cputype;
	int err;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	err = __stm32mp_get_cpu_type(&cputype);
	if (!err && cputype == CPU_STM32MP157Axx)
		fdt = __dtb_z_stm32mp157a_dk1_start;
	else
		fdt = __dtb_z_stm32mp157c_dk2_start;

	stm32mp1_barebox_entry(fdt + get_runtime_offset());
}
