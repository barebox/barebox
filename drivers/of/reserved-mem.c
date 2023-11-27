// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2021 Rouven Czerwinski <r.czerwinski@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "of-reserved-mem: " fmt

#include <stdio.h>
#include <of.h>
#include <of_address.h>
#include <memory.h>
#include <linux/ioport.h>

#define MEMRESERVE_NCELLS	2

static void request_region(struct resource *r)
{
	struct memory_bank *bank;

	for_each_memory_bank(bank) {
		if (!resource_contains(bank->res, r))
			continue;

		pr_debug("reserving %s at %pad-%pad\n", r->name, &r->start, &r->end);

		if (!reserve_sdram_region(r->name, r->start, resource_size(r)))
			pr_warn("couldn't request reserved sdram region %pa-%pa\n",
				&r->start, &r->end);
		break;
	}
}

static int of_reserved_mem_walk(void)
{
	struct device_node *node, *child;
	int ncells = 0;
	const __be32 *reg;

	node = of_find_node_by_path("/reserved-memory");
	if (node) {
		for_each_available_child_of_node(node, child) {
			struct resource resource = {};

			/* skip e.g. linux,cma */
			if (!of_get_property(child, "reg", NULL))
				continue;

			of_address_to_resource(child, 0, &resource);

			resource.name = child->name;
			resource.flags = IORESOURCE_MEM;

			request_region(&resource);
		}
	}

	node = of_find_node_by_path("/memreserve");
	reg = of_get_property(node, "reg", &ncells);
	ncells /= sizeof(__be32);
	if (reg) {
		char name[sizeof "fdt-memreserve-4294967295"];
		int i = 0, n = 0;

		while (i < ncells) {
			struct resource resource = {};
			u64 size;

			snprintf(name, sizeof(name), "fdt-memreserve-%u", n++);
			resource.name = name;
			resource.flags = IORESOURCE_MEM;

			resource.start = of_read_number(reg + i, MEMRESERVE_NCELLS);
			i += MEMRESERVE_NCELLS;

			size = of_read_number(reg + i, MEMRESERVE_NCELLS);
			i += MEMRESERVE_NCELLS;

			if (!size)
				continue;

			resource.end = resource.start + size - 1;

			request_region(&resource);
		}
	}

	return 0;
}
postmem_initcall(of_reserved_mem_walk);
