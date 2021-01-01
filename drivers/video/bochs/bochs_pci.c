// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Copyright (c) 2020 Ahmad Fatoum, Pengutronix
/*
 *  PCI driver entry point for VGA with the Bochs VBE / QEMU stdvga extensions.
 */

#include <common.h>
#include <driver.h>
#include <linux/pci.h>
#include "bochs_hw.h"

static int bochs_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	void __iomem *fb_map, *mmio;
	int ret;

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	fb_map	= pci_iomap(pdev, 0);
	mmio	= pci_iomap(pdev, 2);

	return bochs_hw_probe(&pdev->dev, fb_map, mmio);
}

static DEFINE_PCI_DEVICE_TABLE(bochs_pci_tbl) = {
	/* https://github.com/qemu/qemu/blob/master/docs/specs/standard-vga.txt */
	{ PCI_DEVICE(0x1234, 0x1111) },
};

static struct pci_driver bochs_pci_driver = {
	.name		= "bochs-dispi",
	.probe		= bochs_pci_probe,
	.id_table	= bochs_pci_tbl,
};
device_pci_driver(bochs_pci_driver);
