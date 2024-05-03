/* SPDX-License-Identifier: GPL-2.0-only */

#include <of.h>
#include <of_address.h>
#include <driver.h>

static int of_dma_coherent_fixup(struct device_node *root, void *data)
{
	struct device_node *soc_bb = data, *soc_kernel;
	enum dev_dma_coherence coherency;

	if (of_property_read_bool(soc_bb, "dma-coherent"))
		coherency = DEV_DMA_COHERENT;
	else if (of_property_read_bool(soc_bb, "dma-noncoherent"))
		coherency = DEV_DMA_NON_COHERENT;
	else
		coherency = DEV_DMA_COHERENCE_DEFAULT;

	soc_kernel = of_find_node_by_path_from(root, soc_bb->full_name);
	if (!soc_kernel)
		return -ENOENT;

	of_property_write_bool(soc_kernel, "dma-noncoherent", coherency == DEV_DMA_NON_COHERENT);
	of_property_write_bool(soc_kernel, "dma-coherent", coherency == DEV_DMA_COHERENT);

	return 0;
}

static int of_dma_coherent_fixup_register(void)
{
	struct device_node *soc;

	soc = of_find_node_by_path("/soc");
	if (!soc)
		soc = of_find_node_by_path("/soc@0");
	if (!soc)
		soc = of_find_node_by_path("/");
	if (!soc)
		return -ENOENT;

	return of_register_fixup(of_dma_coherent_fixup, soc);
}
coredevice_initcall(of_dma_coherent_fixup_register);
