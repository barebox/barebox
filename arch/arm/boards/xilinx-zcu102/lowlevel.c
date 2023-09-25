// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <debug_ll.h>
#include <asm/barebox-arm.h>

ENTRY_FUNCTION_WITHSTACK(start_zynqmp_zcu102, 0x80000000, x0, x1, x2)
{
	extern char __dtb_z_zynqmp_zcu102_revB_start[];

	/* Assume that the first stage boot loader configured the UART */
	putc_ll('>');

	barebox_arm_entry(0, SZ_2G, runtime_address(__dtb_z_zynqmp_zcu102_revB_start));
}
