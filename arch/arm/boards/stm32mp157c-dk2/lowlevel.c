// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/stm32.h>
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

	arm_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp157c_dk2_start + get_runtime_offset();

	barebox_arm_entry(STM32_DDR_BASE, SZ_512M, fdt);
}
