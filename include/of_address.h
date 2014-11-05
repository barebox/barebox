#ifndef __OF_ADDRESS_H
#define __OF_ADDRESS_H

#include <common.h>
#include <of.h>

struct of_pci_range_parser {
	struct device_node *node;
	const __be32 *range;
	const __be32 *end;
	int np;
	int pna;
};

struct of_pci_range {
	u32 pci_space;
	u64 pci_addr;
	u64 cpu_addr;
	u64 size;
	u32 flags;
};

#define for_each_of_pci_range(parser, range) \
	for (; of_pci_range_parser_one(parser, range);)

static inline void of_pci_range_to_resource(struct of_pci_range *range,
					    struct device_node *np,
					    struct resource *res)
{
	res->flags = range->flags;
	res->start = range->cpu_addr;
	res->end = range->cpu_addr + range->size - 1;
	res->parent = NULL;
	INIT_LIST_HEAD(&res->children);
	INIT_LIST_HEAD(&res->sibling);
	res->name = np->full_name;
}

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

#ifdef CONFIG_OF_PCI

extern int of_pci_range_parser_init(struct of_pci_range_parser *parser,
			struct device_node *node);

extern struct of_pci_range *of_pci_range_parser_one(
			struct of_pci_range_parser *parser,
			struct of_pci_range *range);

#else

static inline int of_pci_range_parser_init(struct of_pci_range_parser *parser,
			struct device_node *node)
{
	return -1;
}

static inline struct of_pci_range *of_pci_range_parser_one(
					struct of_pci_range_parser *parser,
					struct of_pci_range *range)
{
	return NULL;
}
#endif /* CONFIG_OF_PCI */

#endif /* __OF_ADDRESS_H */
