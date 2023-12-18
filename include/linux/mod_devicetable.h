/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Device tables which are exported to userspace via
 * scripts/mod/file2alias.c.  You must keep that file in sync with this
 * header.
 */

#ifndef LINUX_MOD_DEVICETABLE_H
#define LINUX_MOD_DEVICETABLE_H

#include <linux/types.h>
#include <linux/uuid.h>

#define PCI_ANY_ID (~0)

struct pci_device_id {
	__u32 vendor, device;		/* Vendor and device ID or PCI_ANY_ID*/
	__u32 subvendor, subdevice;	/* Subsystem ID's or PCI_ANY_ID */
	__u32 class, class_mask;	/* (class,subclass,prog-if) triplet */
	unsigned long driver_data;	/* Data private to the driver */
};

#define SPI_NAME_SIZE 32

struct spi_device_id {
	const char *name;
	unsigned long driver_data;
};

/**
 * struct tee_client_device_id - tee based device identifier
 * @uuid: For TEE based client devices we use the device uuid as
 *        the identifier.
 */
struct tee_client_device_id {
	uuid_t uuid;
};

#endif /* LINUX_MOD_DEVICETABLE_H */
