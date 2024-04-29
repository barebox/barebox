// SPDX-License-Identifier: GPL-2.0+
/*
 * PCI <-> OF mapping helpers
 *
 * Copyright 2011 IBM Corp.
 */
#define pr_fmt(fmt)	"PCI: OF: " fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <of.h>
#include <of_address.h>
#include <common.h>
#include <linux/resource_ext.h>

/**
 * of_pci_get_host_bridge_resources() - parsing of PCI host bridge resources from DT
 * @dev: host bridge device
 * @resources: list where the range of resources will be added after DT parsing
 *
 * This function will parse the "ranges" property of a PCI host bridge device
 * node and setup the resource mapping based on its content. It is expected
 * that the property conforms with the Power ePAPR document.
 *
 * It returns zero if the range parsing has been successful or a standard error
 * value if it failed.
 */
static int of_pci_get_host_bridge_resources(struct device *dev,
					    struct pci_controller *bridge)
{
	struct list_head *resources = &bridge->windows;
	struct device_node *dev_node = dev->of_node;
	struct resource *res, tmp_res;
	struct of_pci_range range;
	struct of_pci_range_parser parser;
	const char *range_type;
	int err;

	dev_dbg(dev, "host bridge %pOF ranges:\n", dev_node);

	/* Check for ranges property */
	err = of_pci_range_parser_init(&parser, dev_node);
	if (err)
		return 0;

	dev_dbg(dev, "Parsing ranges property...\n");
	for_each_of_pci_range(&parser, &range) {
		/* Read next ranges element */
		if ((range.flags & IORESOURCE_TYPE_BITS) == IORESOURCE_IO)
			range_type = "IO";
		else if ((range.flags & IORESOURCE_TYPE_BITS) == IORESOURCE_MEM)
			range_type = "MEM";
		else
			range_type = "err";
		dev_dbg(dev, "  %6s %#012llx..%#012llx -> %#012llx\n",
			 range_type, range.cpu_addr,
			 range.cpu_addr + range.size - 1, range.pci_addr);

		/*
		 * If we failed translation or got a zero-sized region
		 * then skip this range
		 */
		if (range.cpu_addr == OF_BAD_ADDR || range.size == 0)
			continue;

		of_pci_range_to_resource(&range, dev_node, &tmp_res);

		res = kmemdup(&tmp_res, sizeof(tmp_res), GFP_KERNEL);
		if (!res) {
			err = -ENOMEM;
			goto failed;
		}

		pci_add_resource_offset(resources, res,	res->start - range.pci_addr);

		switch (res->flags & IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:
			bridge->io_resource = res;
			break;

		case IORESOURCE_MEM:
			if (res->flags & IORESOURCE_PREFETCH)
				bridge->mem_pref_resource = res;
			else
				bridge->mem_resource = res;
			break;
		}
	}

	return 0;

failed:
	return err;
}

int of_pci_bridge_init(struct device *dev, struct pci_controller *bridge)
{
	if (!dev || !dev->of_node)
		return 0;

	return of_pci_get_host_bridge_resources(dev, bridge);
}
