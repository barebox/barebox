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
	struct resource res_core = {}, res_shm = {};
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
		res_core.start = optee_membase;
		res_core.end = optee_membase + OPTEE_SIZE - fixup_data->shm_size - 1;
	} else {
		res_core.start = arm_mem_endmem_get() - OPTEE_SIZE;
		res_core.end = arm_mem_endmem_get() - fixup_data->shm_size - 1;
	}
	res_core.flags = IORESOURCE_MEM | IORESOURCE_BUSY;
	reserve_resource(&res_core);
	res_core.name = "optee_core";

	ret = of_fixup_reserved_memory(root, &res_core);
	if (ret)
		return ret;

	if (!fixup_data->shm_size)
		return 0;

	if (!optee_get_membase(&optee_membase)) {
		res_shm.start = optee_membase + OPTEE_SIZE - fixup_data->shm_size;
		res_shm.end = optee_membase + OPTEE_SIZE - 1;
	} else {
		res_shm.start = arm_mem_endmem_get() - fixup_data->shm_size;
		res_shm.end = arm_mem_endmem_get() - 1;
	}
	res_shm.flags = IORESOURCE_MEM;
	res_shm.type = MEMTYPE_CONVENTIONAL;
	res_shm.attrs = MEMATTRS_RW | MEMATTR_SP;
	res_shm.name = "optee_shm";

	return of_fixup_reserved_memory(root, &res_shm);
}
