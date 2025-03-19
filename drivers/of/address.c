// SPDX-License-Identifier: GPL-2.0-only
/*
 * address.c - address related devicetree functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 */
#include <common.h>
#include <of.h>
#include <of_address.h>

/* Max address size we deal with */
#define OF_MAX_ADDR_CELLS	4
#define OF_CHECK_ADDR_COUNT(na)	((na) > 0 && (na) <= OF_MAX_ADDR_CELLS)
#define OF_CHECK_COUNTS(na, ns)	(OF_CHECK_ADDR_COUNT(na) && (ns) > 0)

/* Debug utility */
#ifdef DEBUG
static void of_dump_addr(const char *s, const __be32 *addr, int na)
{
	printk(KERN_DEBUG "%s", s);
	while (na--)
		printk(" %08x", be32_to_cpu(*(addr++)));
	printk("\n");
}
#else
static void of_dump_addr(const char *s, const __be32 *addr, int na) { }
#endif

/* Callbacks for bus specific translators */
struct of_bus {
	const char	*name;
	const char	*addresses;
	int		(*match)(struct device_node *parent);
	void		(*count_cells)(struct device_node *child,
				       int *addrc, int *sizec);
	u64		(*map)(__be32 *addr, const __be32 *range,
				int na, int ns, int pna);
	int		(*translate)(__be32 *addr, u64 offset, int na);
	unsigned int	(*get_flags)(const __be32 *addr);
};

/*
 * Default translator (generic bus)
 */

static void of_bus_default_count_cells(struct device_node *dev,
				       int *addrc, int *sizec)
{
	if (addrc)
		*addrc = of_n_addr_cells(dev);
	if (sizec)
		*sizec = of_n_size_cells(dev);
}

static u64 of_bus_default_map(__be32 *addr, const __be32 *range,
		int na, int ns, int pna)
{
	u64 cp, s, da;

	cp = of_read_number(range, na);
	s  = of_read_number(range + na + pna, ns);
	da = of_read_number(addr, na);

	pr_vdebug("OF: default map, cp=%llx, s=%llx, da=%llx\n",
		 (unsigned long long)cp, (unsigned long long)s,
		 (unsigned long long)da);

	/*
	 * If the number of address cells is larger than 2 we assume the
	 * mapping doesn't specify a physical address. Rather, the address
	 * specifies an identifier that must match exactly.
	 */
	if (na > 2 && memcmp(range, addr, na * 4) != 0)
		return OF_BAD_ADDR;

	if (da < cp || da >= (cp + s))
		return OF_BAD_ADDR;
	return da - cp;
}

static int of_bus_default_translate(__be32 *addr, u64 offset, int na)
{
	u64 a = of_read_number(addr, na);
	memset(addr, 0, na * 4);
	a += offset;
	if (na > 1)
		addr[na - 2] = cpu_to_be32(a >> 32);
	addr[na - 1] = cpu_to_be32(a & 0xffffffffu);

	return 0;
}

static unsigned int of_bus_default_get_flags(const __be32 *addr)
{
	return IORESOURCE_MEM;
}

#ifdef CONFIG_OF_ADDRESS_PCI
/*
 * PCI bus specific translator
 */

static int of_bus_pci_match(struct device_node *np)
{
	return !of_property_match_string(np, "device_type", "pci");
}

static void of_bus_pci_count_cells(struct device_node *np,
				   int *addrc, int *sizec)
{
	if (addrc)
		*addrc = 3;
	if (sizec)
		*sizec = 2;
}

static unsigned int of_bus_pci_get_flags(const __be32 *addr)
{
	unsigned int flags = 0;
	u32 w = be32_to_cpup(addr);

	switch ((w >> 24) & 0x03) {
	case 0x01:
		flags |= IORESOURCE_IO;
		break;
	case 0x03: /* 64 bits */
		flags |= IORESOURCE_MEM_64;
		fallthrough;
	case 0x02: /* 32 bits */
		flags |= IORESOURCE_MEM;
		break;
	}
	if (w & 0x40000000)
		flags |= IORESOURCE_PREFETCH;
	return flags;
}

static u64 of_bus_pci_map(__be32 *addr, const __be32 *range, int na, int ns,
		int pna)
{
	u64 cp, s, da;
	unsigned int af, rf;

	af = of_bus_pci_get_flags(addr);
	rf = of_bus_pci_get_flags(range);

	/* Check address type match */
	if ((af ^ rf) & (IORESOURCE_MEM | IORESOURCE_IO))
		return OF_BAD_ADDR;

	/* Read address values, skipping high cell */
	cp = of_read_number(range + 1, na - 1);
	s  = of_read_number(range + na + pna, ns);
	da = of_read_number(addr + 1, na - 1);

	pr_vdebug("OF: PCI map, cp=%llx, s=%llx, da=%llx\n",
		 (unsigned long long)cp, (unsigned long long)s,
		 (unsigned long long)da);

	if (da < cp || da >= (cp + s))
		return OF_BAD_ADDR;
	return da - cp;
}

static int of_bus_pci_translate(__be32 *addr, u64 offset, int na)
{
	return of_bus_default_translate(addr + 1, offset, na - 1);
}
#endif /* CONFIG_OF_ADDRESS_PCI */

#ifdef CONFIG_OF_PCI
int of_pci_range_parser_init(struct of_pci_range_parser *parser,
				struct device_node *node)
{
	const int na = 3, ns = 2;
	int rlen;

	parser->node = node;
	parser->pna = of_n_addr_cells(node);
	parser->np = parser->pna + na + ns;

	parser->range = of_get_property(node, "ranges", &rlen);
	if (parser->range == NULL)
		return -ENOENT;

	parser->end = parser->range + rlen / sizeof(__be32);

	return 0;
}
EXPORT_SYMBOL_GPL(of_pci_range_parser_init);

struct of_pci_range *of_pci_range_parser_one(struct of_pci_range_parser *parser,
						struct of_pci_range *range)
{
	const int na = 3, ns = 2;

	if (!range)
		return NULL;

	if (!parser->range || parser->range + parser->np > parser->end)
		return NULL;

	range->pci_space = parser->range[0];
	range->flags = of_bus_pci_get_flags(parser->range);
	range->pci_addr = of_read_number(parser->range + 1, ns);
	range->cpu_addr = of_translate_address(parser->node,
				parser->range + na);
	range->size = of_read_number(parser->range + parser->pna + na, ns);

	parser->range += parser->np;

	/* Now consume following elements while they are contiguous */
	while (parser->range + parser->np <= parser->end) {
		u32 flags, pci_space;
		u64 pci_addr, cpu_addr, size;

		pci_space = be32_to_cpup(parser->range);
		flags = of_bus_pci_get_flags(parser->range);
		pci_addr = of_read_number(parser->range + 1, ns);
		cpu_addr = of_translate_address(parser->node,
				parser->range + na);
		size = of_read_number(parser->range + parser->pna + na, ns);

		if (flags != range->flags)
			break;
		if (pci_addr != range->pci_addr + range->size ||
		    cpu_addr != range->cpu_addr + range->size)
			break;

		range->size += size;
		parser->range += parser->np;
	}

	return range;
}
EXPORT_SYMBOL_GPL(of_pci_range_parser_one);
#endif /* CONFIG_OF_PCI */

/*
 * Array of bus specific translators
 */

static struct of_bus of_busses[] = {
#ifdef CONFIG_OF_ADDRESS_PCI
	/* PCI */
	{
		.name = "pci",
		.addresses = "assigned-addresses",
		.match = of_bus_pci_match,
		.count_cells = of_bus_pci_count_cells,
		.map = of_bus_pci_map,
		.translate = of_bus_pci_translate,
		.get_flags = of_bus_pci_get_flags,
	},
#endif
	/* Default */
	{
		.name = "default",
		.addresses = "reg",
		.match = NULL,
		.count_cells = of_bus_default_count_cells,
		.map = of_bus_default_map,
		.translate = of_bus_default_translate,
		.get_flags = of_bus_default_get_flags,
	},
};

static struct of_bus *of_match_bus(struct device_node *np)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(of_busses); i++)
		if (!of_busses[i].match || of_busses[i].match(np))
			return &of_busses[i];
	BUG();
	return NULL;
}

static int of_translate_one(struct device_node *parent, struct of_bus *bus,
			    struct of_bus *pbus, __be32 *addr,
			    int na, int ns, int pna, const char *rprop)
{
	const __be32 *ranges;
	unsigned int rlen;
	int rone;
	u64 offset = OF_BAD_ADDR;

	/* Normally, an absence of a "ranges" property means we are
	 * crossing a non-translatable boundary, and thus the addresses
	 * below the current not cannot be converted to CPU physical ones.
	 * Unfortunately, while this is very clear in the spec, it's not
	 * what Apple understood, and they do have things like /uni-n or
	 * /ht nodes with no "ranges" property and a lot of perfectly
	 * useable mapped devices below them. Thus we treat the absence of
	 * "ranges" as equivalent to an empty "ranges" property which means
	 * a 1:1 translation at that level. It's up to the caller not to try
	 * to translate addresses that aren't supposed to be translated in
	 * the first place. --BenH.
	 *
	 * As far as we know, this damage only exists on Apple machines, so
	 * This code is only enabled on powerpc. --gcl
	 *
	 * This quirk also applies for 'dma-ranges' which frequently exist in
	 * child nodes without 'dma-ranges' in the parent nodes. --RobH
	 */
	ranges = of_get_property(parent, rprop, &rlen);
#if !defined(CONFIG_PPC)
	if (ranges == NULL && strcmp(rprop, "dma-ranges")) {
		pr_vdebug("OF: no ranges; cannot translate\n");
		return 1;
	}
#endif /* !defined(CONFIG_PPC) */
	if (ranges == NULL || rlen == 0) {
		offset = of_read_number(addr, na);
		memset(addr, 0, pna * 4);
		pr_vdebug("OF: empty ranges; 1:1 translation\n");
		goto finish;
	}

	pr_vdebug("OF: walking ranges...\n");

	/* Now walk through the ranges */
	rlen /= 4;
	rone = na + pna + ns;
	for (; rlen >= rone; rlen -= rone, ranges += rone) {
		offset = bus->map(addr, ranges, na, ns, pna);
		if (offset != OF_BAD_ADDR)
			break;
	}
	if (offset == OF_BAD_ADDR) {
		pr_vdebug("OF: not found !\n");
		return 1;
	}
	memcpy(addr, ranges + na, 4 * pna);

 finish:
	of_dump_addr("OF: parent translation for:", addr, pna);
	pr_vdebug("OF: with offset: %llx\n", (unsigned long long)offset);

	/* Translate it into parent bus space */
	return pbus->translate(addr, offset, pna);
}

/*
 * Translate an address from the device-tree into a CPU physical address,
 * this walks up the tree and applies the various bus mappings on the
 * way.
 *
 * Note: We consider that crossing any level with #size-cells == 0 to mean
 * that translation is impossible (that is we are not dealing with a value
 * that can be mapped to a cpu physical address). This is not really specified
 * that way, but this is traditionally the way IBM at least do things
 */
static u64 __of_translate_address(struct device_node *dev,
				  const __be32 *in_addr, const char *rprop)
{
	struct device_node *parent = NULL;
	struct of_bus *bus, *pbus;
	__be32 addr[OF_MAX_ADDR_CELLS];
	int na, ns, pna, pns;
	u64 result = OF_BAD_ADDR;

	pr_vdebug("OF: ** translation for device %pOF **\n", dev);

	/* Get parent & match bus type */
	parent = of_get_parent(dev);
	if (parent == NULL)
		return OF_BAD_ADDR;
	bus = of_match_bus(parent);

	/* Count address cells & copy address locally */
	bus->count_cells(dev, &na, &ns);
	if (!OF_CHECK_COUNTS(na, ns)) {
		pr_vdebug("prom_parse: Bad cell count for %pOF\n", dev);
		return OF_BAD_ADDR;
	}
	memcpy(addr, in_addr, na * 4);

	pr_vdebug("OF: bus is %s (na=%d, ns=%d) on %pOF\n",
	    bus->name, na, ns, parent);
	of_dump_addr("OF: translating address:", addr, na);

	/* Translate */
	for (;;) {
		/* Switch to parent bus */
		dev = parent;
		parent = of_get_parent(dev);

		/* If root, we have finished */
		if (parent == NULL) {
			pr_vdebug("OF: reached root node\n");
			result = of_read_number(addr, na);
			break;
		}

		/* Get new parent bus and counts */
		pbus = of_match_bus(parent);
		pbus->count_cells(dev, &pna, &pns);
		if (!OF_CHECK_COUNTS(pna, pns)) {
			printk(KERN_ERR "prom_parse: Bad cell count for %pOF\n", dev);
			break;
		}

		pr_vdebug("OF: parent bus is %s (na=%d, ns=%d) on %pOF\n",
		    pbus->name, pna, pns, parent);

		/* Apply bus translation */
		if (of_translate_one(dev, bus, pbus, addr, na, ns, pna, rprop))
			break;

		/* Complete the move up one level */
		na = pna;
		ns = pns;
		bus = pbus;

		of_dump_addr("OF: one level translation:", addr, na);
	}

	return result;
}

u64 of_translate_address(struct device_node *dev, const __be32 *in_addr)
{
	return __of_translate_address(dev, in_addr, "ranges");
}
EXPORT_SYMBOL(of_translate_address);

u64 of_translate_dma_address(struct device_node *dev, const __be32 *in_addr)
{
	return __of_translate_address(dev, in_addr, "dma-ranges");
}
EXPORT_SYMBOL(of_translate_dma_address);

bool of_can_translate_address(struct device_node *dev)
{
	struct device_node *parent;
	struct of_bus *bus;
	int na, ns;

	parent = of_get_parent(dev);
	if (parent == NULL)
		return false;

	bus = of_match_bus(parent);
	bus->count_cells(dev, &na, &ns);

	return OF_CHECK_COUNTS(na, ns);
}
EXPORT_SYMBOL(of_can_translate_address);

static struct device_node *__of_get_dma_parent(struct device_node *np)
{
	struct of_phandle_args args;
	int ret, index;

	index = of_property_match_string(np, "interconnect-names", "dma-mem");
	if (index < 0)
		return of_get_parent(np);

	ret = of_parse_phandle_with_args(np, "interconnects",
					 "#interconnect-cells",
					 index, &args);
	if (ret < 0)
		return of_get_parent(np);

	return args.np;
}

static struct device_node *of_get_next_dma_parent(struct device_node *np)
{
	struct device_node *parent;

	parent = __of_get_dma_parent(np);

	return parent;
}

const __be32 *of_get_address(struct device_node *dev, int index, u64 *size,
		    unsigned int *flags)
{
	const __be32 *prop;
	unsigned int psize;
	struct device_node *parent;
	struct of_bus *bus;
	int onesize, i, na, ns;

	/* Get parent & match bus type */
	parent = of_get_parent(dev);
	if (parent == NULL)
		return NULL;
	bus = of_match_bus(parent);
	bus->count_cells(dev, &na, &ns);
	if (!OF_CHECK_ADDR_COUNT(na))
		return NULL;

	/* Get "reg" or "assigned-addresses" property */
	prop = of_get_property(dev, bus->addresses, &psize);
	if (prop == NULL)
		return NULL;
	psize /= 4;

	onesize = na + ns;
	for (i = 0; psize >= onesize; psize -= onesize, prop += onesize, i++)
		if (i == index) {
			if (size)
				*size = of_read_number(prop + na, ns);
			if (flags)
				*flags = bus->get_flags(prop);
			return prop;
		}
	return NULL;
}
EXPORT_SYMBOL(of_get_address);

static int __of_address_to_resource(struct device_node *dev,
		const __be32 *addrp, u64 size, unsigned int flags,
		const char *name, struct resource *r)
{
	u64 taddr;

	if ((flags & (IORESOURCE_IO | IORESOURCE_MEM)) == 0)
		return -EINVAL;
	taddr = of_translate_address(dev, addrp);
	if (taddr == OF_BAD_ADDR)
		return -EINVAL;
	memset(r, 0, sizeof(struct resource));
	if (flags & IORESOURCE_IO) {
		unsigned long port;
		port = pci_address_to_pio(taddr);
		if (port == (unsigned long)-1)
			return -EINVAL;
		r->start = port;
		r->end = port + size - 1;
	} else {
		r->start = taddr;
		r->end = taddr + size - 1;
	}
	r->flags = flags;
	r->name = name ? name : dev->full_name;

	return 0;
}

/**
 * of_address_to_resource - Translate device tree address and return as resource
 *
 * Note that if your address is a PIO address, the conversion will fail if
 * the physical address can't be internally converted to an IO token with
 * pci_address_to_pio(), that is because it's either called to early or it
 * can't be matched to any host bridge IO space
 */
int of_address_to_resource(struct device_node *dev, int index,
			   struct resource *r)
{
	const __be32	*addrp;
	u64		size;
	unsigned int	flags;
	const char	*name = NULL;

	addrp = of_get_address(dev, index, &size, &flags);
	if (addrp == NULL)
		return -EINVAL;

	/* Get optional "reg-names" property to add a name to a resource */
	of_property_read_string_index(dev, "reg-names",	index, &name);

	return __of_address_to_resource(dev, addrp, size, flags, name, r);
}
EXPORT_SYMBOL_GPL(of_address_to_resource);

struct device_node *of_find_matching_node_by_address(struct device_node *from,
					const struct of_device_id *matches,
					u64 base_address)
{
	struct device_node *dn = of_find_matching_node(from, matches);
	struct resource res;

	while (dn) {
		if (of_address_to_resource(dn, 0, &res))
			continue;
		if (res.start == base_address)
			return dn;
		dn = of_find_matching_node(dn, matches);
	}

	return NULL;
}

/**
 * of_iomap - Maps the memory mapped IO for a given device_node
 * @device:	the device whose io range will be mapped
 * @index:	index of the io range
 *
 * Returns a pointer to the mapped memory
 */
void __iomem *of_iomap(struct device_node *np, int index)
{
	struct resource res;

	if (of_address_to_resource(np, index, &res))
		return NULL;

	return IOMEM(res.start);
}
EXPORT_SYMBOL(of_iomap);

/**
 * of_dma_get_range - Get DMA range info
 * @np:		device node to get DMA range info
 * @dma_addr:	pointer to store initial DMA address of DMA range
 * @paddr:	pointer to store initial CPU address of DMA range
 * @size:	pointer to store size of DMA range
 *
 * Look in bottom up direction for the first "dma-ranges" property
 * and parse it.
 *  dma-ranges format:
 *	DMA addr (dma_addr)	: naddr cells
 *	CPU addr (phys_addr_t)	: pna cells
 *	size			: nsize cells
 *
 * It returns -ENODEV if "dma-ranges" property was not found
 * for this device in DT.
 */
int of_dma_get_range(struct device_node *np, u64 *dma_addr, u64 *paddr, u64 *size)
{
	struct device_node *node = np;
	const __be32 *ranges = NULL;
	int len, naddr, nsize, pna;
	int ret = 0;
	bool found_dma_ranges = false;
	u64 dmaaddr;

	while (node) {
		ranges = of_get_property(node, "dma-ranges", &len);

		/* Ignore empty ranges, they imply no translation required */
		if (ranges && len > 0)
			break;

		/* Once we find 'dma-ranges', then a missing one is an error */
		if (found_dma_ranges && !ranges) {
			ret = -ENODEV;
			goto out;
		}
		found_dma_ranges = true;

		node = of_get_next_dma_parent(node);
	}

	if (!node || !ranges) {
		pr_debug("no dma-ranges found for node(%pOF)\n", np);
		ret = -ENODEV;
		goto out;
	}

	naddr = of_bus_n_addr_cells(node);
	nsize = of_bus_n_size_cells(node);
	pna = of_n_addr_cells(node);
	if ((len / sizeof(__be32)) % (pna + naddr + nsize)) {
		ret = -EINVAL;
		goto out;
	}

	/* dma-ranges format:
	 * DMA addr	: naddr cells
	 * CPU addr	: pna cells
	 * size		: nsize cells
	 */
	dmaaddr = of_read_number(ranges, naddr);
	*paddr = of_translate_dma_address(node, ranges + naddr);
	if (*paddr == OF_BAD_ADDR) {
		pr_err("translation of DMA address(%llx) to CPU address failed node(%s)\n",
		       dmaaddr, np->name);
		ret = -EINVAL;
		goto out;
	}
	*dma_addr = dmaaddr;

	*size = of_read_number(ranges + naddr + pna, nsize);

	pr_debug("dma_addr(%llx) cpu_addr(%llx) size(%llx)\n",
		 *dma_addr, *paddr, *size);

out:

	return ret;
}
