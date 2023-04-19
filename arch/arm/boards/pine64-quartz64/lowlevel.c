// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>

extern char __dtb_rk3566_quartz64_a_start[];

ENTRY_FUNCTION(start_quartz64a, r0, r1, r2)
{
	putc_ll('>');

	if (current_el() == 3)
		relocate_to_adr_full(RK3568_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3568_barebox_entry(__dtb_rk3566_quartz64_a_start);
}
