/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_SAM92_LL_H__
#define __MACH_SAM92_LL_H__

#include <debug_ll.h>
#include <common.h>

#include <mach/at91/at91_pmc_ll.h>
#include <mach/at91/at91sam9260.h>
#include <mach/at91/at91sam9261.h>
#include <mach/at91/at91sam9263.h>
#include <mach/at91/at91sam926x.h>
#include <mach/at91/debug_ll.h>
#include <mach/at91/early_udelay.h>
#include <mach/at91/iomux.h>

void sam9263_lowlevel_init(u32 plla, u32 pllb);

static inline void sam92_pmc_enable_periph_clock(int clk)
{
	at91_pmc_enable_periph_clock(IOMEM(AT91SAM926X_BASE_PMC), clk);
}

/* requires relocation */
static inline void sam92_udelay_init(unsigned int msc)
{
	early_udelay_init(IOMEM(AT91SAM926X_BASE_PMC), IOMEM(AT91SAM9263_BASE_PIT),
			  AT91SAM926X_ID_SYS, msc, 0);
}

static inline void sam92_dbgu_setup_ll(unsigned int mck)
{
	void __iomem *pio = IOMEM(AT91SAM9263_BASE_PIOC);

	// Setup clock for pio
	sam92_pmc_enable_periph_clock(AT91SAM9263_ID_PIOCDE);

	// Setup DBGU uart
	at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PC30), AT91_MUX_PERIPH_A, GPIO_PULL_UP); // DRXD
	at91_mux_pio_pin(pio, pin_to_mask(AT91_PIN_PC31), AT91_MUX_PERIPH_A, 0); // DTXD

	// Setup dbgu
	at91_dbgu_setup_ll(IOMEM(AT91_BASE_DBGU1), mck, CONFIG_BAUDRATE);
	pbl_set_putc(at91_dbgu_putc, IOMEM(AT91_BASE_DBGU1));
	putc_ll('#');
}

#endif
