// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 David Jander, Protonic Holland

#include <common.h>
#include <debug_ll.h>
#include <mach/stm32mp/entry.h>

extern char __dtb_z_stm32mp151_prtt1a_start[];
extern char __dtb_z_stm32mp151_prtt1c_start[];
extern char __dtb_z_stm32mp151_prtt1s_start[];
extern char __dtb_z_stm32mp151_mecio1_start[];
extern char __dtb_z_stm32mp151_mect1s_start[];
extern char __dtb_z_stm32mp151c_plyaqm_start[];

static void setup_uart(void)
{
	/* first stage has set up the UART, so nothing to do here */
	putc_ll('>');
}

ENTRY_FUNCTION(start_prtt1a, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp151_prtt1a_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_prtt1c, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp151_prtt1c_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_prtt1s, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp151_prtt1s_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_mecio1, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp151_mecio1_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_mect1s, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_stm32mp151_mect1s_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_stm32mp151c_plyaqm, r0, r1, r2)
{
	void *fdt;

	stm32mp_cpu_lowlevel_init();

	setup_uart();

	fdt = __dtb_z_stm32mp151c_plyaqm_start + get_runtime_offset();

	stm32mp1_barebox_entry(fdt);
}
