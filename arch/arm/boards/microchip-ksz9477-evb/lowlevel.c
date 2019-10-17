// SPDX-License-Identifier: GPL-2.0-only AND BSD-1-Clause
/*
 * Copyright (C) 2014, Atmel Corporation
 * Copyright (C) 2018 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/at91_pmc_ll.h>

#include <mach/hardware.h>
#include <mach/iomux.h>
#include <debug_ll.h>
#include <mach/at91_dbgu.h>

/* PCK = 528MHz, MCK = 132MHz */
#define MASTER_CLOCK	132000000

#define sama5d3_pmc_enable_periph_clock(clk) \
	at91_pmc_enable_periph_clock(IOMEM(SAMA5D3_BASE_PMC), clk)

static void dbgu_init(void)
{
	void __iomem *pio = IOMEM(SAMA5D3_BASE_PIOB);

	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_PIOB);

	at91_mux_pio3_pin(pio, pin_to_mask(AT91_PIN_PB31), AT91_MUX_PERIPH_A, 0);

	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_DBGU);
	at91_dbgu_setup_ll(IOMEM(AT91_BASE_DBGU1), MASTER_CLOCK, 115200);

	putc_ll('>');
}

extern char __dtb_z_at91_microchip_ksz9477_evb_start[];

ENTRY_FUNCTION(start_sama5d3_xplained_ung8071, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	arm_setup_stack(SAMA5D3_SRAM_BASE + SAMA5D3_SRAM_SIZE);

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		dbgu_init();

	fdt = __dtb_z_at91_microchip_ksz9477_evb_start + get_runtime_offset();

	barebox_arm_entry(SAMA5_DDRCS, SZ_256M, fdt);
}
