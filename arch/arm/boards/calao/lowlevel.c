// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <debug_ll.h>
#include <asm/reloc.h>
#include <mach/at91/barebox-arm.h>
#include <mach/at91/at91sam9_sdramc.h>
#include <mach/at91/at91sam9260.h>
#include <mach/at91/hardware.h>

static void dbgu_init(void)
{
	/* pinmux/clocks/uart already configured by first stage */
	putc_ll('>');
}

#define CALAO_ENTRY_2ND(entrypoint, dtbname) \
AT91_ENTRY_FUNCTION(entrypoint, r0, r1, r2) { \
	extern char __dtb_z_##dtbname##_start[]; \
	arm_cpu_lowlevel_init(); \
	arm_setup_stack(AT91SAM9260_SRAM_END); \
	dbgu_init(); \
	at91sam9260_barebox_entry(runtime_address(__dtb_z_##dtbname##_start)); \
}

CALAO_ENTRY_2ND(start_tny_a9260, tny_a9260);
CALAO_ENTRY_2ND(start_tny_a9g20, tny_a9g20);
CALAO_ENTRY_2ND(start_usb_a9260, usb_a9260);
CALAO_ENTRY_2ND(start_usb_a9g20, usb_a9g20);
