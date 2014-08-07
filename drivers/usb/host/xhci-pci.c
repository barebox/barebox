/*
 * PCI driver for xHCI controllers
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/pci.h>
#include <usb/xhci.h>

static int xhci_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct xhci_data data = {};

	pci_enable_device(pdev);
	pci_set_master(pdev);

	data.regs = pci_iomap(pdev, 0);

	return xhci_register(&pdev->dev, &data);
}

static DEFINE_PCI_DEVICE_TABLE(xhci_pci_tbl) = {
	/* handle any USB 3.0 xHCI controller */
	{ PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_XHCI, ~0), },
	{ },
};

static struct pci_driver xhci_pci_driver = {
	.name = "xHCI PCI",
	.id_table = xhci_pci_tbl,
	.probe = xhci_pci_probe,
};

static int xhci_pci_init(void)
{
	return pci_register_driver(&xhci_pci_driver);
}
device_initcall(xhci_pci_init);
