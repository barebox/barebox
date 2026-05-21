/* SPDX-License-Identifier: GPL-2.0-only */
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

#ifdef CONFIG_PCI_DYNAMIC_OF_NODES
void of_pci_make_dev_node(struct pci_dev *pdev);
#else
static inline void of_pci_make_dev_node(struct pci_dev *pdev) { }
#endif

#endif
