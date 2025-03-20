// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <mach/at91/barebox-arm.h>
#include <mach/at91/sama5d2_ll.h>
#include <mach/at91/iomux.h>
#include <mach/at91/xload.h>
#include <debug_ll.h>
#include <mach/at91/sama5d2-sip-ddramc.h>

#define RGB_LED_GREEN (1 << 0)
#define RGB_LED_RED   (1 << 1)
#define RGB_LED_BLUE  (1 << 2)

/* PCK = 492MHz, MCK = 164MHz */
#define MASTER_CLOCK	164000000

static void ek_turn_led(unsigned color)
{
	struct {
		void __iomem *pio;
		unsigned bit;
		unsigned color;
	} *led, leds[] = {
		{ .pio = SAMA5D2_BASE_PIOA, .bit = 10, .color = color & RGB_LED_RED },
		{ .pio = SAMA5D2_BASE_PIOB, .bit =  1, .color = color & RGB_LED_GREEN },
		{ .pio = SAMA5D2_BASE_PIOA, .bit = 31, .color = color & RGB_LED_BLUE },
		{ /* sentinel */ },
	};

	for (led = leds; led->pio; led++) {
		at91_mux_gpio4_enable(led->pio, BIT(led->bit));
		at91_mux_gpio4_input(led->pio, BIT(led->bit), false);
		at91_mux_gpio4_set(led->pio, BIT(led->bit), led->color);
	}
}

SAMA5D2_ENTRY_FUNCTION(start_sama5d27_som1_ek_xload, r4)
{
	void __iomem *dbgu_base;
	sama5d2_lowlevel_init();

	dbgu_base = sama5d2_resetup_uart_console(MASTER_CLOCK);
	putc_ll('>');

	relocate_to_current_adr();
	setup_c();

	pbl_set_putc(at91_dbgu_putc, dbgu_base);

	ek_turn_led(RGB_LED_RED | RGB_LED_GREEN); /* Yellow */
	sama5d2_udelay_init(MASTER_CLOCK);
	sama5d2_d1g_ddrconf();
	sama5d2_start_image(r4);
}

extern char __dtb_z_at91_sama5d27_som1_ek_start[];

SAMA5D2_ENTRY_FUNCTION(start_sama5d27_som1_ek, r4)
{
	void *fdt;

	putc_ll('>');

	fdt = __dtb_z_at91_sama5d27_som1_ek_start + get_runtime_offset();

	ek_turn_led(RGB_LED_GREEN);
	sama5d2_barebox_entry(r4, fdt);
}
