// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <mach/at91/barebox-arm.h>
#include <mach/at91/sama5d2_ll.h>
#include <mach/at91/xload.h>
#include <mach/at91/sama5d2-sip-ddramc.h>
#include <mach/at91/iomux.h>
#include <debug_ll.h>

/* PCK = 492MHz, MCK = 164MHz */
#define MASTER_CLOCK	164000000

SAMA5D2_ENTRY_FUNCTION(start_sama5d27_giantboard_xload, r4)
{
	void __iomem *dbgu_base;

	sama5d2_lowlevel_init();

	dbgu_base = sama5d2_resetup_uart_console(MASTER_CLOCK);
	putc_ll('>');

	relocate_to_current_adr();
	setup_c();

	pbl_set_putc(at91_dbgu_putc, dbgu_base);

	sama5d2_udelay_init(MASTER_CLOCK);
	sama5d2_d1g_ddrconf();
	sama5d2_start_image(r4);
}

extern char __dtb_z_at91_sama5d27_giantboard_start[];

SAMA5D2_ENTRY_FUNCTION(start_sama5d27_giantboard, r4)
{
	void *fdt;

	putc_ll('>');

	fdt = __dtb_z_at91_sama5d27_giantboard_start + get_runtime_offset();

	sama5d2_barebox_entry(r4, fdt);
}
