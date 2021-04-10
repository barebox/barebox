// SPDX-License-Identifier: GPL-2.0
#include <linux/libfdt.h>
#include <pbl.h>
#include <printk.h>

void fdt_find_mem(const void *fdt, unsigned long *membase, unsigned long *memsize)
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

const void *fdt_device_get_match_data(const void *fdt, const char *nodepath,
				      const struct fdt_device_id ids[])
{
	int node, length;
	const char *list, *end;
	const struct fdt_device_id *id;

	node = fdt_path_offset(fdt, nodepath);
	if (node < 0)
		return NULL;

	list = fdt_getprop(fdt, node, "compatible", &length);
	if (!list)
		return NULL;

	end = list + length;

	while (list < end) {
		length = strnlen(list, end - list) + 1;

		/* Abort if the last string isn't properly NUL-terminated. */
		if (list + length > end)
			return NULL;

		for (id = ids; id->compatible; id++) {
			if (!strcasecmp(list, id->compatible))
				return id->data;
		}

		list += length;
	}

	return NULL;
}
