// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Ahmad Fatoum
 */

#ifndef __ACPI_H_
#define __ACPI_H_

#include <linux/string.h>
#include <linux/types.h>
#include <driver.h>
#include <efi/efi-init.h>

/* Names within the namespace are 4 bytes long */

#define ACPI_NAMESEG_SIZE               4	/* Fixed by ACPI spec */
#define ACPI_PATH_SEGMENT_LENGTH        5	/* 4 chars for name + 1 char for separator */
#define ACPI_PATH_SEPARATOR             '.'

/* Sizes for ACPI table headers */

#define ACPI_OEM_ID_SIZE                6
#define ACPI_OEM_TABLE_ID_SIZE          8

/*
 * Algorithm to obtain access bit or byte width.
 * Can be used with access_width of struct acpi_generic_address and access_size of
 * struct acpi_resource_generic_register.
 */
#define ACPI_ACCESS_BIT_WIDTH(size)     (1 << ((size) + 2))
#define ACPI_ACCESS_BYTE_WIDTH(size)    (1 << ((size) - 1))

/* Address Space (Operation Region) Types */

typedef u8 acpi_adr_space_type;
#define ACPI_ADR_SPACE_SYSTEM_MEMORY    (acpi_adr_space_type) 0
#define ACPI_ADR_SPACE_SYSTEM_IO        (acpi_adr_space_type) 1
#define ACPI_ADR_SPACE_PCI_CONFIG       (acpi_adr_space_type) 2
#define ACPI_ADR_SPACE_EC               (acpi_adr_space_type) 3
#define ACPI_ADR_SPACE_SMBUS            (acpi_adr_space_type) 4
#define ACPI_ADR_SPACE_CMOS             (acpi_adr_space_type) 5
#define ACPI_ADR_SPACE_PCI_BAR_TARGET   (acpi_adr_space_type) 6
#define ACPI_ADR_SPACE_IPMI             (acpi_adr_space_type) 7
#define ACPI_ADR_SPACE_GPIO             (acpi_adr_space_type) 8
#define ACPI_ADR_SPACE_GSBUS            (acpi_adr_space_type) 9
#define ACPI_ADR_SPACE_PLATFORM_COMM    (acpi_adr_space_type) 10
#define ACPI_ADR_SPACE_PLATFORM_RT      (acpi_adr_space_type) 11

/*******************************************************************************
 *
 * Master ACPI Table Header. This common header is used by all ACPI tables
 * except the RSDP and FACS.
 *
 ******************************************************************************/

struct __packed acpi_table_header {
	char signature[ACPI_NAMESEG_SIZE];	/* ASCII table signature */
	u32 length;		/* Length of table in bytes, including this header */
	u8 revision;		/* ACPI Specification minor version number */
	u8 checksum;		/* To make sum of entire table == 0 */
	char oem_id[ACPI_OEM_ID_SIZE];	/* ASCII OEM identification */
	char oem_table_id[ACPI_OEM_TABLE_ID_SIZE];	/* ASCII OEM table identification */
	u32 oem_revision;	/* OEM revision number */
	char asl_compiler_id[ACPI_NAMESEG_SIZE];	/* ASCII ASL compiler vendor ID */
	u32 asl_compiler_revision;	/* ASL compiler version */
};

/*******************************************************************************
 *
 * GAS - Generic Address Structure (ACPI 2.0+)
 *
 * Note: Since this structure is used in the ACPI tables, it is byte aligned.
 * If misaligned access is not supported by the hardware, accesses to the
 * 64-bit Address field must be performed with care.
 *
 ******************************************************************************/

struct __packed acpi_generic_address {
	u8 space_id;		/* Address space where struct or register exists */
	u8 bit_width;		/* Size in bits of given register */
	u8 bit_offset;		/* Bit offset within the register */
	u8 access_width;	/* Minimum Access size (ACPI 3.0) */
	u64 address;		/* 64-bit address of struct or register */
};

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
	struct driver driver;
	acpi_sig_t signature;
};

extern struct bus_type acpi_bus;

static inline struct acpi_driver *to_acpi_driver(struct driver *drv)
{
	return container_of(drv, struct acpi_driver, driver);
}

#define device_acpi_driver(drv)	\
	register_efi_driver_macro(device, acpi, drv)

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
