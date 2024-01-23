/* SPDX-License-Identifier: GPL-2.0-only */

#include <of.h>
#include <linux/ioport.h>
#include <asm/barebox-arm.h>
#include <asm/optee.h>
#include <tee/optee.h>

int of_optee_fixup(struct device_node *root, void *_data)
{
	struct of_optee_fixup_data *fixup_data = _data;
	const char *optee_of_path = "/firmware/optee";
	struct resource res = {};
	struct device_node *node;
	u64 optee_membase;
	int ret;

	if (of_find_node_by_path_from(root, optee_of_path))
		return 0;

	node = of_create_node(root, optee_of_path);
	if (!node)
		return -ENOMEM;

	ret = of_property_write_string(node, "compatible", "linaro,optee-tz");
	if (ret)
		return ret;

	ret = of_property_write_string(node, "method", fixup_data->method);
	if (ret)
		return ret;

	if (!optee_get_membase(&optee_membase)) {
		res.start = optee_membase;
		res.end = optee_membase + OPTEE_SIZE - fixup_data->shm_size - 1;
	} else {
		res.start = arm_mem_endmem_get() - OPTEE_SIZE;
		res.end = arm_mem_endmem_get() - fixup_data->shm_size - 1;
	}
	res.flags = IORESOURCE_BUSY;
	res.name = "optee_core";

	ret = of_fixup_reserved_memory(root, &res);
	if (ret)
		return ret;

	if (!optee_get_membase(&optee_membase)) {
		res.start = optee_membase + OPTEE_SIZE - fixup_data->shm_size;
		res.end = optee_membase + OPTEE_SIZE - 1;
	} else {
		res.start = arm_mem_endmem_get() - fixup_data->shm_size;
		res.end = arm_mem_endmem_get() - 1;
	}
	res.flags &= ~IORESOURCE_BUSY;
	res.name = "optee_shm";

	return of_fixup_reserved_memory(root, &res);
}
