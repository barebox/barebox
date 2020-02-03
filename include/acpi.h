// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Ahmad Fatoum
 */

#ifndef __ACPI_H_
#define __ACPI_H_

#include <linux/string.h>
#include <linux/types.h>
#include <driver.h>

typedef char acpi_sig_t[4];

struct __packed acpi_rsdp { /* root system description pointer */
	char	signature[8];
	u8	checksum;
	u8	oem_id[6];
	u8	revision;
	u32	rsdt_addr;
};

struct __packed acpi2_rsdp { /* root system description */
	struct acpi_rsdp acpi1;
	u32	length;
	u64	xsdt_addr;
	u8	extended_checksum;
	u8	reserved[3];
};

struct __packed acpi_sdt { /* system description table header */
	acpi_sig_t	signature;
	u32		len;
	u8		revision;
	u8		checksum;
	char		oem_id[6];
	char		oem_table_id[8];
	u32		oem_revision;
	u32		creator_id;
	u32		creator_revision;
};

struct __packed acpi_rsdt { /* system description table header */
	struct acpi_sdt	sdt;
	struct acpi_sdt	* __aligned(8) entries[];
};

struct acpi_driver {
	struct driver_d driver;
	acpi_sig_t signature;
};

extern struct bus_type acpi_bus;

static inline struct acpi_driver *to_acpi_driver(struct driver_d *drv)
{
	return container_of(drv, struct acpi_driver, driver);
}

#define device_acpi_driver(drv)	\
	register_driver_macro(device, acpi, drv)

static inline int acpi_driver_register(struct acpi_driver *acpidrv)
{
	acpidrv->driver.bus = &acpi_bus;
	return register_driver(&acpidrv->driver);
}

static inline int acpi_sigcmp(const acpi_sig_t sig_a, const acpi_sig_t sig_b)
{
	return memcmp(sig_a, sig_b, sizeof(acpi_sig_t));
}

#endif
