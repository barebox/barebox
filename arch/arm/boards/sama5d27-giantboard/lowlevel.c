// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <mach/barebox-arm.h>
#include <mach/sama5d2_ll.h>
#include <mach/iomux.h>
#include <debug_ll.h>
#include <mach/at91_dbgu.h>

/* PCK = 492MHz, MCK = 164MHz */
#define MASTER_CLOCK	164000000

static void dbgu_init(void)
{
	sama5d2_resetup_uart_console(MASTER_CLOCK);

	putc_ll('>');
}

extern char __dtb_z_at91_sama5d27_giantboard_start[];

SAMA5_ENTRY_FUNCTION(start_sama5d27_giantboard, r4)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		dbgu_init();

	fdt = __dtb_z_at91_sama5d27_giantboard_start + get_runtime_offset();

	sama5d2_barebox_entry(r4, fdt);
}
