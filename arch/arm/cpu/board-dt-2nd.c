// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <pbl.h>

#ifdef CONFIG_CPU_V8

/* called from assembly */
void dt_2nd_aarch64(void *fdt);

void dt_2nd_aarch64(void *fdt)
{
	unsigned long membase, memsize;

	/* entry point already set up stack */

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	if (!fdt)
		hang();

	fdt_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

#else

static noinline void dt_2nd_continue(void *fdt)
{
	unsigned long membase, memsize;

	if (!fdt)
		hang();

	fdt_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

ENTRY_FUNCTION(start_dt_2nd, r0, r1, r2)
{
	unsigned long image_start = (unsigned long)_text + global_variable_offset();

	arm_cpu_lowlevel_init();

	arm_setup_stack(image_start);

	relocate_to_current_adr();
	setup_c();
	barrier();

	dt_2nd_continue((void *)r2);
}
#endif
