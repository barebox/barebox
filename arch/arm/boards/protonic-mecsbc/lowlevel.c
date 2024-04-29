// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>

extern char __dtb_rk3568_mecsbc_start[];

ENTRY_FUNCTION(start_mecsbc, r0, r1, r2)
{
	/*
	 * MECSBC IO domain voltages are all +3.3V, except VCCIO4 and VCCIO6
	 * Both GMAC interfaces need this to work properly.
	 * FIXME: This is done by the io-domain driver as well, but there
	 * currently is no mechanism to make sure the driver gets probed
	 * before its consumers. Remove this setup once this issue is
	 * resolved.
	 */
	writel(RK_SETBITS(0x50), 0xfdc20140);

	putc_ll('>');

	if (current_el() == 3)
		relocate_to_adr_full(RK3568_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3568_barebox_entry(__dtb_rk3568_mecsbc_start);
}
