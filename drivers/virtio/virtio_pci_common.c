// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Virtio PCI driver - common functionality for all device versions
 *
 * This module allows virtio devices to be used over a virtual PCI device.
 * This can be used with QEMU based VMMs like KVM or Xen.
 *
 * Copyright IBM Corp. 2007
 * Copyright Red Hat, Inc. 2014
 *
 * Authors:
 *  Anthony Liguori  <aliguori@us.ibm.com>
 *  Rusty Russell <rusty@rustcorp.com.au>
 *  Michael S. Tsirkin <mst@redhat.com>
 */

#include <common.h>
#include <init.h>
#include "virtio_pci_common.h"

/* Qumranet donated their vendor ID for devices 0x1000 thru 0x10FF. */
static const struct pci_device_id virtio_pci_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_REDHAT_QUMRANET, PCI_ANY_ID) },
	{ 0 }
};

static int virtio_pci_probe(struct pci_dev *pci_dev,
			    const struct pci_device_id *id)
{
	struct virtio_pci_device *vp_dev;
	int rc;

	/* allocate our structure and fill it out */
	vp_dev = kzalloc(sizeof(struct virtio_pci_device), GFP_KERNEL);
	if (!vp_dev)
		return -ENOMEM;

	pci_dev->dev.priv = vp_dev;
	vp_dev->vdev.dev.parent = &pci_dev->dev;
	vp_dev->pci_dev = pci_dev;

	/* enable the device */
	rc = pci_enable_device(pci_dev);
	if (rc)
		goto err_enable_device;

	rc = virtio_pci_modern_probe(vp_dev);
	if (rc == -ENODEV)
		dev_err(&pci_dev->dev, "Legacy devices unsupported\n");
	if (rc)
		goto err_enable_device;

	pci_set_master(pci_dev);

	rc = register_virtio_device(&vp_dev->vdev);
	if (rc)
		goto err_probe;

	return 0;

err_probe:
	pci_clear_master(pci_dev);
err_enable_device:
	kfree(vp_dev);
	return rc;
}

static void virtio_pci_remove(struct pci_dev *pci_dev)
{
	struct virtio_pci_device *vp_dev = pci_dev->dev.priv;

	unregister_virtio_device(&vp_dev->vdev);

	pci_clear_master(pci_dev);
}

static struct pci_driver virtio_pci_driver = {
	.name		= "virtio-pci",
	.id_table	= virtio_pci_id_table,
	.probe		= virtio_pci_probe,
	.remove		= virtio_pci_remove,
};

device_pci_driver(virtio_pci_driver);

MODULE_AUTHOR("Anthony Liguori <aliguori@us.ibm.com>");
MODULE_DESCRIPTION("virtio-pci");
MODULE_LICENSE("GPL");
