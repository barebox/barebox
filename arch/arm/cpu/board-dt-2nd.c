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

/* Use the appended device tree on raw entry or if none is passed */
static void *dt_2nd_fdt(void *fdt, bool raw_entry)
{
	if (blob_is_fdt(__image_end) &&
	    (raw_entry || !fdt || !blob_is_fdt(fdt)))
		return __image_end;

	return fdt;
}

#ifdef CONFIG_CPU_V8

/* called from assembly */
void dt_2nd_aarch64(void *fdt, bool raw_entry);

void dt_2nd_aarch64(void *fdt, bool raw_entry)
{
	unsigned long membase, memsize;

	putc_ll('>');

	/* entry point already set up stack */

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	fdt = dt_2nd_fdt(fdt, raw_entry);
	if (!fdt)
		hang();

	fdt_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

#else

static noinline void dt_2nd_continue(void *fdt)
{
	unsigned long membase, memsize;

	fdt = dt_2nd_fdt(fdt, false);
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
