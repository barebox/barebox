// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>

extern char __dtb_rk3576_prtpuk_start[];

ENTRY_FUNCTION(start_prtpuk, r0, r1, r2)
{
	putc_ll('>');

	relocate_to_current_adr();

	setup_c();

	rk3576_barebox_entry(__dtb_rk3576_prtpuk_start);
}
