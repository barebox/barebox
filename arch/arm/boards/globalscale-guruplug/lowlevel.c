// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/lowlevel.h>

extern char __dtb_kirkwood_guruplug_server_plus_bb_start[];

ENTRY_FUNCTION(start_globalscale_guruplug, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_kirkwood_guruplug_server_plus_bb_start +
		get_runtime_offset();

	kirkwood_barebox_entry(fdt);
}
