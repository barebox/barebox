// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
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

#define RGB_LED_GREEN (1 << 0)
#define RGB_LED_RED   (1 << 1)
#define RGB_LED_BLUE  (1 << 2)

/* PCK = 492MHz, MCK = 164MHz */
#define MASTER_CLOCK	164000000

static inline void sama5d2_pmc_enable_periph_clock(int clk)
{
	at91_pmc_sam9x5_enable_periph_clock(IOMEM(SAMA5D2_BASE_PMC), clk);
}

static void ek_turn_led(unsigned color)
{
	struct {
		unsigned long pio;
		unsigned bit;
		unsigned color;
	} *led, leds[] = {
		{ .pio = SAMA5D2_BASE_PIOA, .bit = 10, .color = color & RGB_LED_RED },
		{ .pio = SAMA5D2_BASE_PIOB, .bit =  1, .color = color & RGB_LED_GREEN },
		{ .pio = SAMA5D2_BASE_PIOA, .bit = 31, .color = color & RGB_LED_BLUE },
		{ /* sentinel */ },
	};

	for (led = leds; led->pio; led++) {
		at91_mux_gpio4_enable(IOMEM(led->pio), BIT(led->bit));
		at91_mux_gpio4_input(IOMEM(led->pio), BIT(led->bit), false);
		at91_mux_gpio4_set(IOMEM(led->pio), BIT(led->bit), led->color);
	}
}

static void ek_dbgu_init(void)
{
	unsigned mck = MASTER_CLOCK / 2;

	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_PIOD);

	at91_mux_pio4_set_A_periph(IOMEM(SAMA5D2_BASE_PIOD),
				   pin_to_mask(AT91_PIN_PD3)); /* DBGU TXD */

	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_UART1);

	at91_dbgu_setup_ll(IOMEM(SAMA5D2_BASE_UART1), mck, 115200);

	putc_ll('>');
}

extern char __dtb_z_at91_sama5d27_som1_ek_start[];

static noinline void som1_entry(void)
{
	void *fdt;

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		ek_dbgu_init();

	fdt = __dtb_z_at91_sama5d27_som1_ek_start + get_runtime_offset();

	ek_turn_led(RGB_LED_GREEN);
	barebox_arm_entry(SAMA5_DDRCS, SZ_128M, fdt);
}

ENTRY_FUNCTION(start_sama5d27_som1_ek, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	arm_setup_stack(SAMA5D2_SRAM_BASE + SAMA5D2_SRAM_SIZE);

	som1_entry();
}
