// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <linux/libfdt.h>

static void of_find_mem(void *fdt, unsigned long *membase, unsigned long *memsize)
{
	const __be32 *nap, *nsp, *reg;
	uint32_t na, ns;
	uint64_t memsize64, membase64;
	int node, size, i;

	/* Make sure FDT blob is sane */
	if (fdt_check_header(fdt) != 0) {
		pr_err("Invalid device tree blob\n");
		goto err;
	}

	/* Find the #address-cells and #size-cells properties */
	node = fdt_path_offset(fdt, "/");
	if (node < 0) {
		pr_err("Cannot find root node\n");
		goto err;
	}

	nap = fdt_getprop(fdt, node, "#address-cells", &size);
	if (!nap || (size != 4)) {
		pr_err("Cannot find #address-cells property");
		goto err;
	}
	na = fdt32_to_cpu(*nap);

	nsp = fdt_getprop(fdt, node, "#size-cells", &size);
	if (!nsp || (size != 4)) {
		pr_err("Cannot find #size-cells property");
		goto err;
	}
	ns = fdt32_to_cpu(*nap);

	/* Find the memory range */
	node = fdt_node_offset_by_prop_value(fdt, -1, "device_type",
					     "memory", sizeof("memory"));
	if (node < 0) {
		pr_err("Cannot find memory node\n");
		goto err;
	}

	reg = fdt_getprop(fdt, node, "reg", &size);
	if (size < (na + ns) * sizeof(u32)) {
		pr_err("cannot get memory range\n");
		goto err;
	}

	membase64 = 0;
	for (i = 0; i < na; i++)
		membase64 = (membase64 << 32) | fdt32_to_cpu(*reg++);

	/* get the memsize and truncate it to under 4G on 32 bit machines */
	memsize64 = 0;
	for (i = 0; i < ns; i++)
		memsize64 = (memsize64 << 32) | fdt32_to_cpu(*reg++);

	*membase = membase64;
	*memsize = memsize64;

	return;
err:
	pr_err("No memory, cannot continue\n");
	while (1);
}

#ifdef CONFIG_CPU_V8

static noinline void dt_2nd_continue_aarch64(void *fdt)
{
	unsigned long membase, memsize;

	if (!fdt)
		hang();

	of_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

/* called from assembly */
void dt_2nd_aarch64(void *fdt);

void dt_2nd_aarch64(void *fdt)
{
	unsigned long image_start = (unsigned long)_text + global_variable_offset();

	arm_setup_stack(image_start);

	relocate_to_current_adr();
	setup_c();

	dt_2nd_continue_aarch64(fdt);
}

#else

static noinline void dt_2nd_continue(void *fdt)
{
	unsigned long membase, memsize;

	if (!fdt)
		hang();

	of_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

ENTRY_FUNCTION(start_dt_2nd, r0, r1, r2)
{
	unsigned long image_start = (unsigned long)_text + global_variable_offset();

	arm_setup_stack(image_start);

	relocate_to_current_adr();
	setup_c();
	barrier();

	dt_2nd_continue((void *)r2);
}
#endif
