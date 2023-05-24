// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>
#include <mach/rockchip/rockchip.h>

extern char __dtb_rk3588_rock_5b_start[];

ENTRY_FUNCTION(start_rock5b, r0, r1, r2)
{
	putc_ll('>');

	if (current_el() == 3)
		relocate_to_adr_full(RK3588_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3588_barebox_entry(__dtb_rk3588_rock_5b_start);
}
