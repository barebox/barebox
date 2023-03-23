// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>
#include <mach/rockchip/rockchip.h>

extern char __dtb_rk3568_bpi_r2_pro_start[];

ENTRY_FUNCTION(start_rk3568_bpi_r2pro, r0, r1, r2)
{
	putc_ll('>');

	/*
	 * set iodomain vccio6 to 1.8V needed for GMAC1 to work.
	 * vccio4 (gmac0/switch) needs to stay at 3v3 (default)
	 * FIXME: This is done by the io-domain driver as well, but there
	 * currently is no mechanism to make sure the driver gets probed
	 * before its consumers. Remove this setup once this issue is
	 * resolved.
	 */
	//set bit 6 in PMU_GRF_IO_VSEL0 for vccio6 1v8
	writel(RK_SETBITS(BIT(6)), PMU_GRF_IO_VSEL0);
	//clear bit 6 for 3v3 as it was set to 1v8
	writel(RK_CLRBITS(BIT(6)), PMU_GRF_IO_VSEL1);

	if (current_el() == 3)
		relocate_to_adr_full(RK3568_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3568_barebox_entry(__dtb_rk3568_bpi_r2_pro_start);
}
