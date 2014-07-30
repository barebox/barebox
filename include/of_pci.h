#ifndef __OF_PCI_H
#define __OF_PCI_H

#include <linux/pci.h>

#ifdef CONFIG_OF_PCI
int of_pci_get_devfn(struct device_node *np);

#else
static inline int of_pci_get_devfn(struct device_node *np)
{
	return -EINVAL;
}

#endif

#endif
