// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/lowlevel.h>

extern char __dtb_kirkwood_openblocks_a6_bb_start[];

ENTRY_FUNCTION(start_plathome_openblocks_a6, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_kirkwood_openblocks_a6_bb_start +
		get_runtime_offset();

	kirkwood_barebox_entry(fdt);
}
