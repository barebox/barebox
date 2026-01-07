// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/rockchip/hardware.h>
#include <mach/rockchip/atf.h>
#include <debug_ll.h>
#include <mach/rockchip/rockchip.h>

extern char __dtb_rk3568_qnap_ts433_start[];
extern char __dtb_rk3568_qnap_ts433eu_start[];

ENTRY_FUNCTION(start_rk3568_qnap_ts433, r0, r1, r2)
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

	rk3568_barebox_entry(__dtb_rk3568_qnap_ts433_start);
}

/*
 * There is an EEPROM that we can use for identification (TS433-eU
 * is indicated by "QA011" string in EEPROM), but we don't have an
 * I2C driver in PBL yet for this platform, so we keep a second image
 * for now.
 */
ENTRY_FUNCTION(start_rk3568_qnap_ts433eu, r0, r1, r2)
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

	rk3568_barebox_entry(__dtb_rk3568_qnap_ts433eu_start);
}
