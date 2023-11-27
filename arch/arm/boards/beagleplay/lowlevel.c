// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <debug_ll.h>
#include <pbl.h>

/* Called from assembly */
void beagleplay(void);

static noinline void beagleplay_continue(void)
{
	unsigned long membase, memsize;
	extern char __dtb_k3_am625_beagleplay_start[];

	fdt_find_mem(__dtb_k3_am625_beagleplay_start, &membase, &memsize);

	barebox_arm_entry(membase, memsize, __dtb_k3_am625_beagleplay_start);
}

void beagleplay(void)
{
	putc_ll('>');

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();

	setup_c();

	beagleplay_continue();
}
