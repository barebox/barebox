// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2021 Rouven Czerwinski <r.czerwinski@pengutronix.de>, Pengutronix

#include <stdio.h>
#include <of.h>
#include <of_address.h>

#define MEMRESERVE_NCELLS	2
#define MEMRESERVE_FLAGS	(IORESOURCE_MEM | IORESOURCE_EXCLUSIVE)

int of_reserved_mem_walk(int (*handler)(const struct resource *res))
{
	struct device_node *node, *child;
	int ncells = 0;
	const __be32 *reg;
	int ret;

	node = of_find_node_by_path("/reserved-memory");
	if (node) {
		for_each_available_child_of_node(node, child) {
			struct resource resource = {};

			/* skip e.g. linux,cma */
			if (!of_get_property(child, "reg", NULL))
				continue;

			of_address_to_resource(child, 0, &resource);

			resource.name = child->name;
			resource.flags = MEMRESERVE_FLAGS;

			ret = handler(&resource);
			if (ret)
				return ret;
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
			resource.flags = MEMRESERVE_FLAGS;

			resource.start = of_read_number(reg + i, MEMRESERVE_NCELLS);
			i += MEMRESERVE_NCELLS;

			size = of_read_number(reg + i, MEMRESERVE_NCELLS);
			i += MEMRESERVE_NCELLS;

			if (!size)
				continue;

			resource.end = resource.start + size - 1;

			ret = handler(&resource);
			if (ret)
				return ret;
		}
	}

	return 0;
}
