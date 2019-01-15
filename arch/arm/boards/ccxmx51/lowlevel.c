/* SPDX-License-Identifier: GPL-2.0+ */
/* Author: Alexander Shiyan <shc_work@mail.ru> */

#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx51-regs.h>

ENTRY_FUNCTION(start_ccxmx51, r0, r1, r2)
{
	extern char __dtb_imx51_ccxmx51_start[];
	void *fdt;

	imx5_cpu_lowlevel_init();

	arm_setup_stack(0x20000000 - 16);

	fdt = __dtb_imx51_ccxmx51_start + get_runtime_offset();

	barebox_arm_entry(MX51_CSD0_BASE_ADDR, SZ_128M, fdt);
}
