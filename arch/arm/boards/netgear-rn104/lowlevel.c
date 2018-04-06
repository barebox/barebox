/*
 * Copyright (C) 2014  Uwe Kleine-Koenig <uwe@kleine-koenig.org>
 */

#include <common.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/lowlevel.h>

extern char __dtb_armada_370_rn104_bb_start[];

ENTRY_FUNCTION(start_netgear_rn104, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_armada_370_rn104_bb_start +
		get_runtime_offset();

	armada_370_xp_barebox_entry(fdt);
}
