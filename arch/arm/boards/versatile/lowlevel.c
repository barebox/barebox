// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_versatile_pb_start[];

ENTRY_FUNCTION(start_versatile_pb, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_versatile_pb_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_64M, fdt);
}
