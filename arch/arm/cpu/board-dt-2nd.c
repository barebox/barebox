// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <compressed-dtb.h>
#include <pbl.h>

/* Without a device tree passed, use one appended to the image file */
static void *dt_2nd_fdt(void *fdt)
{
	if ((!fdt || !blob_is_fdt(fdt)) && blob_is_fdt(__image_end))
		return __image_end;

	return fdt;
}

#ifdef CONFIG_CPU_V8

/* called from assembly */
void dt_2nd_aarch64(void *fdt);

void dt_2nd_aarch64(void *fdt)
{
	unsigned long membase, memsize;

	putc_ll('>');

	/* entry point already set up stack */

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	fdt = dt_2nd_fdt(fdt);
	if (!fdt)
		hang();

	fdt_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

#else

static noinline void dt_2nd_continue(void *fdt)
{
	unsigned long membase, memsize;

	fdt = dt_2nd_fdt(fdt);
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
