// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/lowlevel.h>

extern char __dtb_armada_xp_openblocks_ax3_4_bb_start[];

ENTRY_FUNCTION(start_plathome_openblocks_ax3, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_armada_xp_openblocks_ax3_4_bb_start +
		get_runtime_offset();

	armada_370_xp_barebox_entry(fdt);
}
