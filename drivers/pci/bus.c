// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/pci.h>

/**
 * pci_match_one_device - Tell if a PCI device structure has a matching
 *                        PCI device id structure
 * @id: single PCI device id structure to match
 * @dev: the PCI device structure to match against
 *
 * Returns the matching pci_device_id structure or %NULL if there is no match.
 */
static inline const struct pci_device_id *
pci_match_one_device(const struct pci_device_id *id, const struct pci_dev *dev)
{
	if ((id->vendor == PCI_ANY_ID || id->vendor == dev->vendor) &&
	    (id->device == PCI_ANY_ID || id->device == dev->device) &&
	    (id->subvendor == PCI_ANY_ID || id->subvendor == dev->subsystem_vendor) &&
	    (id->subdevice == PCI_ANY_ID || id->subdevice == dev->subsystem_device) &&
	    !((id->class ^ dev->class) & id->class_mask))
		return id;
	return NULL;
}

/**
 * pci_match_id - See if a pci device matches a given pci_id table
 * @ids: array of PCI device id structures to search in
 * @dev: the PCI device structure to match against.
 *
 * Used by a driver to check whether a PCI device present in the
 * system is in its list of supported devices.  Returns the matching
 * pci_device_id structure or %NULL if there is no match.
 *
 * Deprecated, don't use this as it will not catch any dynamic ids
 * that a driver might want to check for.
 */
const struct pci_device_id *pci_match_id(const struct pci_device_id *ids,
					 struct pci_dev *dev)
{
	if (ids) {
		while (ids->vendor || ids->subvendor || ids->class_mask) {
			if (pci_match_one_device(ids, dev))
				return ids;
			ids++;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(pci_match_id);

static int pci_match(struct device *dev, struct driver *drv)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_driver *pdrv = to_pci_driver(drv);
	const struct pci_device_id *id;

	for (id = pdrv->id_table; id->vendor; id++)
		if (pci_match_one_device(id, pdev)) {
			pdev->id = id;
			return 0;
		}

	return -1;
}

static int pci_probe(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_driver *pdrv = to_pci_driver(dev->driver);

	return pdrv->probe(pdev, pdev->id);
}

static void pci_remove(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_driver *pdrv = to_pci_driver(dev->driver);

	if (pdrv->remove)
		pdrv->remove(pdev);
}

struct bus_type pci_bus = {
	.name = "pci",
	.match = pci_match,
	.probe = pci_probe,
	.remove = pci_remove,
};

static int pci_bus_init(void)
{
	return bus_register(&pci_bus);
}
pure_initcall(pci_bus_init);

int pci_register_driver(struct pci_driver *pdrv)
{
	struct driver *drv = &pdrv->driver;

	if (!pdrv->id_table)
		return -EIO;

	drv->name = pdrv->name;
	drv->bus = &pci_bus;

	return register_driver(drv);
}

int pci_register_device(struct pci_dev *pdev)
{
	char str[6];
	struct device *dev = &pdev->dev;
	int ret;

	if (!of_device_is_available(pdev->dev.of_node))
		return 0;

	dev_set_name(dev, "pci-%04x:%04x.", pdev->vendor, pdev->device);
	dev->bus = &pci_bus;
	dev->id = DEVICE_ID_DYNAMIC;

	ret = register_device(dev);

	if (ret)
		return ret;

	sprintf(str, "%02x", pdev->devfn);
	dev_add_param_fixed(dev, "devfn", str);
	sprintf(str, "%04x", (pdev->class >> 8) & 0xffff);
	dev_add_param_fixed(dev, "class", str);
	sprintf(str, "%04x", pdev->vendor);
	dev_add_param_fixed(dev, "vendor", str);
	sprintf(str, "%04x", pdev->device);
	dev_add_param_fixed(dev, "device", str);
	sprintf(str, "%04x", pdev->revision);
	dev_add_param_fixed(dev, "revision", str);

	return 0;
}
