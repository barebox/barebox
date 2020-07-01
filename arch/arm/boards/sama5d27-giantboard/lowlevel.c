// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/sama5d2_ll.h>
#include <mach/iomux.h>
#include <debug_ll.h>
#include <mach/at91_dbgu.h>

/* PCK = 492MHz, MCK = 164MHz */
#define MASTER_CLOCK	164000000

static void dbgu_init(void)
{
	unsigned mck = MASTER_CLOCK / 2;

	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_PIOD);

	at91_mux_pio4_set_A_periph(SAMA5D2_BASE_PIOD,
				   pin_to_mask(AT91_PIN_PD3)); /* DBGU TXD */

	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_UART1);

	at91_dbgu_setup_ll(SAMA5D2_BASE_UART1, mck, 115200);

	putc_ll('>');
}

extern char __dtb_z_at91_sama5d27_giantboard_start[];

static noinline void giantboard_entry(void)
{
	void *fdt;

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		dbgu_init();

	fdt = __dtb_z_at91_sama5d27_giantboard_start + get_runtime_offset();

	barebox_arm_entry(SAMA5_DDRCS, SZ_128M, fdt);
}

ENTRY_FUNCTION(start_sama5d27_giantboard, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(SAMA5D2_SRAM_BASE + SAMA5D2_SRAM_SIZE);

	giantboard_entry();
}
