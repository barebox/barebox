// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/reset.h>
#include <module.h>

#include "xhci.h"

static const char hcd_name[] = "xhci_hcd";

static int xhci_pci_common_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct xhci_ctrl *ctrl;
	void __iomem *base;
	int ret;

	(void)id;

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	pci_set_master(pdev);

	base = pci_iomap(pdev, 0);
	if (!base) {
		ret = dev_err_probe(dev, -EBUSY, "failed to map BAR0\n");
		goto err_clear_master;
	}

	ctrl = xzalloc(sizeof(*ctrl));
	ctrl->dev = dev;
	ctrl->hccr = base;
	ctrl->hcor = (struct xhci_hcor *)((uintptr_t)ctrl->hccr +
		HC_LENGTH(xhci_readl(&(ctrl->hccr)->cr_capbase)));

	dev->priv = ctrl;

	ret = xhci_register(ctrl);
	if (ret)
		goto err_free_ctrl;

	return 0;

err_free_ctrl:
	dev->priv = NULL;
	free(ctrl);
err_clear_master:
	pci_clear_master(pdev);
	return ret;
}

static int xhci_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	return xhci_pci_common_probe(dev, id);
}

static void xhci_pci_remove(struct pci_dev *dev)
{
	struct xhci_ctrl *ctrl = dev->dev.priv;

	xhci_deregister(ctrl);
	pci_clear_master(dev);
	free(ctrl);
}

/*-------------------------------------------------------------------------*/

/* PCI driver selection metadata; PCI hotplugging uses this */
static const struct pci_device_id pci_ids[] = {
	/* handle any USB 3.0 xHCI controller */
	{ PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_XHCI, PCI_ANY_ID),
	},
	{ /* end: all zeroes */ }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

/* pci driver glue; this is a "new style" PCI driver module */
static struct pci_driver xhci_pci_driver = {
	.name =		hcd_name,
	.id_table =	pci_ids,

	.probe =	xhci_pci_probe,
	.remove =	xhci_pci_remove,
};

static int __init xhci_pci_init(void)
{
	return pci_register_driver(&xhci_pci_driver);
}
module_init(xhci_pci_init);

MODULE_DESCRIPTION("xHCI PCI Host Controller Driver");
MODULE_LICENSE("GPL");
