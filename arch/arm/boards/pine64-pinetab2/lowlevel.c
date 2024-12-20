// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>

extern char __dtb_rk3566_pinetab2_v0_1_start[];
extern char __dtb_rk3566_pinetab2_v2_0_start[];

ENTRY_FUNCTION(start_pinetab2_v0, r0, r1, r2)
{
	putc_ll('>');

	if (current_el() == 3)
		relocate_to_adr_full(RK3568_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3568_barebox_entry(__dtb_rk3566_pinetab2_v0_1_start);
}

ENTRY_FUNCTION(start_pinetab2_v2, r0, r1, r2)
{
	putc_ll('>');

	if (current_el() == 3)
		relocate_to_adr_full(RK3568_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3568_barebox_entry(__dtb_rk3566_pinetab2_v2_0_start);
}
