/* SPDX-License-Identifier: GPL-2.0-only */

#include <of.h>
#include <of_address.h>
#include <driver.h>

static int of_dma_coherent_fixup(struct device_node *root, void *data)
{
	struct device_node *soc;
	enum dev_dma_coherence coherency = (enum dev_dma_coherence)(uintptr_t)data;

	soc = of_find_node_by_path_from(root, "/soc");
	if (!soc)
		return -ENOENT;

	of_property_write_bool(soc, "dma-noncoherent", coherency == DEV_DMA_NON_COHERENT);
	of_property_write_bool(soc, "dma-coherent", coherency == DEV_DMA_COHERENT);

	return 0;
}

static int of_dma_coherent_fixup_register(void)
{
	struct device_node *soc;
	enum dev_dma_coherence soc_dma_coherency;

	soc = of_find_node_by_path("/soc");
	if (!soc)
		return -ENOENT;

	if (of_property_read_bool(soc, "dma-coherent"))
		soc_dma_coherency = DEV_DMA_COHERENT;
	else if (of_property_read_bool(soc, "dma-noncoherent"))
		soc_dma_coherency = DEV_DMA_NON_COHERENT;
	else
		soc_dma_coherency = DEV_DMA_COHERENCE_DEFAULT;

	return of_register_fixup(of_dma_coherent_fixup, (void *)(uintptr_t)soc_dma_coherency);
}
coredevice_initcall(of_dma_coherent_fixup_register);
