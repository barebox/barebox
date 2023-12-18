/* SPDX-License-Identifier: GPL-2.0-only */

#include <of.h>
#include <linux/ioport.h>
#include <asm/barebox-arm.h>
#include <asm/optee.h>

int of_optee_fixup(struct device_node *root, void *_data)
{
	struct of_optee_fixup_data *fixup_data = _data;
	struct resource res = {};
	struct device_node *node;
	int ret;

	node = of_create_node(root, "/firmware/optee");
	if (!node)
		return -ENOMEM;

	ret = of_property_write_string(node, "compatible", "linaro,optee-tz");
	if (ret)
		return ret;

	ret = of_property_write_string(node, "method", fixup_data->method);
	if (ret)
		return ret;

	res.start = arm_mem_endmem_get() - OPTEE_SIZE;
	res.end = arm_mem_endmem_get() - fixup_data->shm_size -1;
	res.flags = IORESOURCE_BUSY;
	res.name = "optee_core";

	ret = of_fixup_reserved_memory(root, &res);
	if (ret)
		return ret;

	res.start = arm_mem_endmem_get() - fixup_data->shm_size;
	res.end = arm_mem_endmem_get() - 1;
	res.flags &= ~IORESOURCE_BUSY;
	res.name = "optee_shm";

	return of_fixup_reserved_memory(root, &res);
}
