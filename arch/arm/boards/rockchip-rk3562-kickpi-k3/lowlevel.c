// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>

extern char __dtb_rk3562_kickpi_k3_start[];

ENTRY_FUNCTION(start_rk3562_kickpi_k3, r0, r1, r2)
{
	putc_ll('>');

	if (current_el() == 3)
		relocate_to_adr_full(RK3562_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3562_barebox_entry(__dtb_rk3562_kickpi_k3_start);
}
