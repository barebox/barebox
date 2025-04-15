// SPDX-License-Identifier: GPL-2.0-only AND BSD-1-Clause
/*
 * Copyright (C) 2014, Atmel Corporation
 * Copyright (C) 2018 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <debug_ll.h>
#include <mach/at91/barebox-arm.h>
#include <mach/at91/iomux.h>
#include <mach/at91/sama5d3.h>
#include <mach/at91/sama5d3-xplained-ddramc.h>
#include <mach/at91/xload.h>

/* PCK = 528MHz, MCK = 132MHz */
#define MASTER_CLOCK	132000000

static void dbgu_init(void)
{
	void __iomem *pio = IOMEM(SAMA5D3_BASE_PIOB);

	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_PIOB);

	at91_mux_pio3_pin(pio, pin_to_mask(AT91_PIN_PB31), AT91_MUX_PERIPH_A, 0);

	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_DBGU);
	at91_dbgu_setup_ll(IOMEM(AT91_BASE_DBGU1), MASTER_CLOCK, 115200);

	putc_ll('>');
	pbl_set_putc(at91_dbgu_putc, IOMEM(AT91_BASE_DBGU1));
}

SAMA5D3_ENTRY_FUNCTION(start_sama5d3_xplained_xload_mmc, r4)
{
	sama5d3_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	dbgu_init();

	sama5d3_udelay_init(MASTER_CLOCK);
	sama5d3_xplained_ddrconf();

	sama5d3_atmci_start_image(0, MASTER_CLOCK, 0);
}

extern char __dtb_z_at91_sama5d3_xplained_start[];

SAMA5D3_ENTRY_FUNCTION(start_sama5d3_xplained, r4)
{
	void *fdt;

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		dbgu_init();

	fdt = __dtb_z_at91_sama5d3_xplained_start + get_runtime_offset();

	sama5d3_barebox_entry(r4, fdt);
}
