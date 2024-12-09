// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <debug_ll.h>
#include <pbl.h>
#include <pbl/handoff-data.h>
#include <compressed-dtb.h>

/* Called from assembly */
void beagleplay(void *dtb);

static noinline void beagleplay_continue(void *dtb)
{
	unsigned long membase, memsize;
	extern char __dtb_k3_am625_beagleplay_start[];
	unsigned int size;

	fdt_find_mem(__dtb_k3_am625_beagleplay_start, &membase, &memsize);

	if (blob_is_valid_fdt_ptr(dtb, membase, memsize, &size))
		handoff_data_add(HANDOFF_DATA_EXTERNAL_DT, dtb, size);

	barebox_arm_entry(membase, memsize, __dtb_k3_am625_beagleplay_start);
}

void beagleplay(void *dtb)
{
	putc_ll('>');

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();

	setup_c();

	beagleplay_continue(dtb);
}
