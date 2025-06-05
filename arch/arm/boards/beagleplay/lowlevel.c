// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/k3/debug_ll.h>
#include <debug_ll.h>
#include <pbl.h>
#include <pbl/handoff-data.h>
#include <compressed-dtb.h>
#include <cache.h>
#include <mach/k3/r5.h>
#include <mach/k3/common.h>

#include "ddr.h"

static noinline void beagleplay_continue(void)
{
	unsigned long membase = 0x80000000, memsize;
	extern char __dtb_k3_am625_beagleplay_start[];

	memsize = am62x_sdram_size();

	barebox_arm_entry(membase, memsize, __dtb_k3_am625_beagleplay_start);
}

ENTRY_FUNCTION_WITHSTACK(start_beagleplay, 0x80800000, r0, r1, r2)
{
	putc_ll('>');

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();

	setup_c();

	beagleplay_continue();
}

extern char __dtb_k3_am625_r5_beagleplay_start[];

static noinline void beagleplay_r5_continue(void)
{
	pbl_set_putc((void *)debug_ll_ns16550_putc, (void *)AM62X_UART_UART0_BASE);

	putc_ll('>');

	k3_mpu_setup_regions();

	am625_early_init();
	beagleplay_ddr_init();

	barebox_arm_entry(0x80000000, 0x80000000, __dtb_k3_am625_r5_beagleplay_start);
}

void beagleplay_r5_entry(void);

void beagleplay_r5_entry(void)
{
	k3_ctrl_mmr_unlock();

	writel(0x00050000, 0xf41c8);
	writel(0x00010000, 0xf41cc);

	k3_debug_ll_init((void *)AM62X_UART_UART0_BASE);

	relocate_to_current_adr();
	setup_c();

	beagleplay_r5_continue();
}
