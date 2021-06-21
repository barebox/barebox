// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/hardware.h>
#include <mach/atf.h>
#include <debug_ll.h>
#include <mach/rockchip.h>

extern char __dtb_rk3568_evb1_v10_start[];

static noinline void rk3568_start(void)
{
	void *fdt;

	/*
	 * Enable vccio4 1.8V and vccio6 1.8V
	 * Needed for GMAC to work.
	 */
	writel(RK_SETBITS(0x50), 0xfdc20140);

	fdt = __dtb_rk3568_evb1_v10_start;

	if (current_el() == 3) {
		rk3568_lowlevel_init();
		rk3568_atf_load_bl31(fdt);
		/* not reached */
	}

	barebox_arm_entry(RK3568_DRAM_BOTTOM, 0x80000000 - RK3568_DRAM_BOTTOM, fdt);
}

ENTRY_FUNCTION(start_rk3568_evb, r0, r1, r2)
{
	/*
	 * Image execution starts at 0x0, but this is used for ATF and
	 * OP-TEE later, so move away from here.
	 */
	if (current_el() == 3)
		relocate_to_adr_full(RK3568_BAREBOX_LOAD_ADDRESS);
	else
		relocate_to_current_adr();

	setup_c();

	rk3568_start();
}
