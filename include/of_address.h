#ifndef __OF_ADDRESS_H
#define __OF_ADDRESS_H

#include <common.h>
#include <of.h>

#ifndef pci_address_to_pio
static inline unsigned long pci_address_to_pio(phys_addr_t addr) { return -1; }
#endif

#ifdef CONFIG_OFTREE

extern u64 of_translate_address(struct device_node *dev,
				const __be32 *in_addr);
extern u64 of_translate_dma_address(struct device_node *dev,
				const __be32 *in_addr);
extern bool of_can_translate_address(struct device_node *dev);
extern const __be32 *of_get_address(struct device_node *dev, int index,
				u64 *size, unsigned int *flags);
extern int of_address_to_resource(struct device_node *dev, int index,
				struct resource *r);
extern struct device_node *of_find_matching_node_by_address(
	struct device_node *from, const struct of_device_id *matches,
	u64 base_address);
extern void __iomem *of_iomap(struct device_node *np, int index);

#else /* CONFIG_OFTREE */

static inline u64 of_translate_address(struct device_node *dev,
				const __be32 *in_addr)
{
	return OF_BAD_ADDR;
}

static inline u64 of_translate_dma_address(struct device_node *dev,
				const __be32 *in_addr)
{
	return OF_BAD_ADDR;
}

static inline bool of_can_translate_address(struct device_node *dev)
{
	return false;
}

static inline const __be32 *of_get_address(struct device_node *dev, int index,
				u64 *size, unsigned int *flags)
{
	return 0;
}

static inline int of_address_to_resource(struct device_node *dev, int index,
				struct resource *r)
{
	return -ENOSYS;
}

static inline struct device_node *of_find_matching_node_by_address(
	struct device_node *from, const struct of_device_id *matches,
	u64 base_address)
{
	return NULL;
}

static inline void __iomem *of_iomap(struct device_node *np, int index)
{
	return NULL;
}

#endif /* CONFIG_OFTREE */

#endif /* __OF_ADDRESS_H */
