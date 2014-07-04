/*
 * Implement the default iomap interfaces
 *
 * (C) Copyright 2004 Linus Torvalds
 */
#include <linux/pci.h>
#include <io.h>

#include <module.h>

/**
 * pci_iomap - create a virtual mapping cookie for a PCI BAR
 * @dev: PCI device that owns the BAR
 * @bar: BAR number
 *
 * Using this function you will get a __iomem address to your device BAR.
 * You can access it using ioread*() and iowrite*(). These functions hide
 * the details if this is a MMIO or PIO address space and will just do what
 * you expect from them in the correct way.
 *
 */
void __iomem *pci_iomap(struct pci_dev *dev, int bar)
{
	struct pci_bus *bus = dev->bus;
	resource_size_t start = pci_resource_start(dev, bar);

	return (void *)bus->ops->res_start(bus, start);
}
EXPORT_SYMBOL(pci_iomap);
