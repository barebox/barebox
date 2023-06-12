// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic PCIE host provided by e.g. QEMU
 *
 * Heavily based on drivers/pci/pcie_xilinx.c
 *
 * Copyright (C) 2016 Imagination Technologies
 */

#include <common.h>
#include <malloc.h>
#include <io.h>
#include <of.h>
#include <of_address.h>
#include <init.h>
#include <linux/pci.h>
#include <linux/sizes.h>

struct generic_ecam_pcie {
	struct pci_controller pci;
	struct resource *cfg;
	int first_busno;
	struct resource io;
	struct resource mem;
	struct resource prefetch;
};

static inline struct generic_ecam_pcie *host_to_ecam(struct pci_controller *host)
{
	return container_of(host, struct generic_ecam_pcie, pci);
}

static void __iomem *pci_generic_ecam_conf_address(const struct pci_bus *bus,
						   u32 devfn, int where)
{
	struct generic_ecam_pcie *ecam = host_to_ecam(bus->host);
	void __iomem *addr;

	addr = IOMEM(ecam->cfg->start);
	addr += (bus->number - ecam->first_busno) << 20;
	addr += PCI_SLOT(devfn) << 15;
	addr += PCI_FUNC(devfn) << 12;
	addr += where;

	return addr;
}

static bool pci_generic_ecam_addr_valid(const struct pci_bus *bus, u32 devfn)
{
	struct generic_ecam_pcie *ecam = host_to_ecam(bus->host);
	int num_buses = DIV_ROUND_UP(resource_size(ecam->cfg), 1 << 16);

	return (bus->number >= ecam->first_busno &&
		bus->number < ecam->first_busno + num_buses);
}

static int pci_generic_ecam_read_config(struct pci_bus *bus,
					u32 devfn, int where,
					int size, u32 *val)
{
	void __iomem *addr;

	if (!pci_generic_ecam_addr_valid(bus, devfn))
		return PCIBIOS_DEVICE_NOT_FOUND;

	addr = pci_generic_ecam_conf_address(bus, devfn, where);

	if (!IS_ALIGNED((uintptr_t)addr, size)) {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	if (size == 4) {
		*val = readl(addr);
	} else if (size == 2) {
		*val = readw(addr);
	} else if (size == 1) {
		*val = readb(addr);
	} else {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	return PCIBIOS_SUCCESSFUL;
}

static int pci_generic_ecam_write_config(struct pci_bus *bus, u32 devfn,
					 int where, int size, u32 val)
{
	void __iomem *addr;

	if (!pci_generic_ecam_addr_valid(bus, devfn))
		return PCIBIOS_DEVICE_NOT_FOUND;

	addr = pci_generic_ecam_conf_address(bus, devfn, where);

	if (!IS_ALIGNED((uintptr_t)addr, size))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (size == 4)
		writel(val, addr);
	else if (size == 2)
		writew(val, addr);
	else if (size == 1)
		writeb(val, addr);
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

static void pcie_ecam_set_local_bus_nr(struct pci_controller *host, int busno)
{
	struct generic_ecam_pcie *ecam = host_to_ecam(host);

	ecam->first_busno = busno;
}

static const struct pci_ops pci_generic_ecam_ops = {
	.read = pci_generic_ecam_read_config,
	.write = pci_generic_ecam_write_config,
};

static inline bool is_64bit(const struct resource *res)
{
	return res->flags & IORESOURCE_MEM_64;
}

static int pcie_ecam_parse_dt(struct generic_ecam_pcie *ecam)
{
	struct device *dev = ecam->pci.parent;
	struct device_node *np = dev->of_node;
	struct of_pci_range_parser parser;
	struct of_pci_range range;
	struct resource res;

	if (of_pci_range_parser_init(&parser, np)) {
		dev_err(dev, "missing \"ranges\" property\n");
		return -EINVAL;
	}

	for_each_of_pci_range(&parser, &range) {
		of_pci_range_to_resource(&range, np, &res);

		switch (res.flags & IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:
			memcpy(&ecam->io, &res, sizeof(res));
			ecam->io.name = "I/O";
			break;

		case IORESOURCE_MEM:
			if (res.flags & IORESOURCE_PREFETCH) {
				memcpy(&ecam->prefetch, &res, sizeof(res));
				ecam->prefetch.name = "PREFETCH";
			} else {
				/* Choose 32-bit mappings over 64-bit ones if possible */
				if (ecam->mem.name && !is_64bit(&ecam->mem) && is_64bit(&res))
					break;

				memcpy(&ecam->mem, &res, sizeof(res));
				ecam->mem.name = "MEM";
			}
			break;
		}
	}

	return 0;
}

static int pcie_ecam_probe(struct device *dev)
{
	struct generic_ecam_pcie *ecam;
	struct resource *iores;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	ecam = xzalloc(sizeof(*ecam));
	ecam->cfg = iores;
	ecam->pci.parent = dev;
	ecam->pci.pci_ops = &pci_generic_ecam_ops;
	ecam->pci.set_busno = pcie_ecam_set_local_bus_nr;
	ecam->pci.mem_resource = &ecam->mem;
	ecam->pci.io_resource = &ecam->io;
	ecam->pci.mem_pref_resource = &ecam->prefetch;

	ret = pcie_ecam_parse_dt(ecam);
	if (ret)
		return ret;

	register_pci_controller(&ecam->pci);
	return 0;
}

static struct of_device_id pcie_ecam_dt_ids[] = {
	{ .compatible = "pci-host-ecam-generic" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, pcie_ecam_dt_ids);

static struct driver pcie_ecam_driver = {
	.name = "pcie-generic-ecam",
	.probe = pcie_ecam_probe,
	.of_compatible = pcie_ecam_dt_ids,
};
device_platform_driver(pcie_ecam_driver);
