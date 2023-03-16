// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <mach/mvebu/barebox-arm-head.h>
#include <mach/mvebu/lowlevel.h>

extern char __dtb_kirkwood_topkick_bb_start[];

ENTRY_FUNCTION_MVEBU(start_usi_topkick, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_kirkwood_topkick_bb_start + get_runtime_offset();

	kirkwood_barebox_entry(fdt);
}
