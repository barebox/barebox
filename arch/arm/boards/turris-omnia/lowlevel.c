/*
 * Copyright (C) 2017 Pengutronix, Uwe Kleine-Koenig <u.kleine-koenig@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/lowlevel.h>

#include <asm/io.h>

extern char __dtb_armada_385_turris_omnia_bb_start[];

ENTRY_FUNCTION(start_turris_omnia, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_armada_385_turris_omnia_bb_start -
		get_runtime_offset();

	armada_370_xp_barebox_entry(fdt);
}
