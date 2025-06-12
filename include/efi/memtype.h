/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_MEMTYPE_H_
#define _EFI_MEMTYPE_H_

#include <linux/ioport.h>
#include <efi/types.h>

/* Memory types: */
enum efi_memory_type {
	EFI_RESERVED_TYPE = MEMTYPE_RESERVED,
	EFI_LOADER_CODE = MEMTYPE_LOADER_CODE,
	EFI_LOADER_DATA = MEMTYPE_LOADER_DATA,
	EFI_BOOT_SERVICES_CODE = MEMTYPE_BOOT_SERVICES_CODE,
	EFI_BOOT_SERVICES_DATA = MEMTYPE_BOOT_SERVICES_DATA,
	EFI_RUNTIME_SERVICES_CODE = MEMTYPE_RUNTIME_SERVICES_CODE,
	EFI_RUNTIME_SERVICES_DATA = MEMTYPE_RUNTIME_SERVICES_DATA,
	EFI_CONVENTIONAL_MEMORY = MEMTYPE_CONVENTIONAL,
	EFI_UNUSABLE_MEMORY = MEMTYPE_UNUSABLE,
	EFI_ACPI_RECLAIM_MEMORY = MEMTYPE_ACPI_RECLAIM,
	EFI_ACPI_MEMORY_NVS = MEMTYPE_ACPI_NVS,
	EFI_MEMORY_MAPPED_IO = MEMTYPE_MMIO,
	EFI_MEMORY_MAPPED_IO_PORT_SPACE = MEMTYPE_MMIO_PORT,
	EFI_PAL_CODE = MEMTYPE_PAL_CODE,
	EFI_PERSISTENT_MEMORY_TYPE = MEMTYPE_PERSISTENT,
	EFI_UNACCEPTED_MEMORY_TYPE = MEMTYPE_UNACCEPTED,
	EFI_MAX_MEMORY_TYPE = MEMTYPE_MAX,
};

/* Attribute values: */
#define EFI_MEMORY_UC		((u64)MEMATTR_UC)	/* uncached */
#define EFI_MEMORY_WC		((u64)MEMATTR_WC)	/* write-coalescing */
#define EFI_MEMORY_WT		((u64)MEMATTR_WT)	/* write-through */
#define EFI_MEMORY_WB		((u64)MEMATTR_WB)	/* write-back */
#define EFI_MEMORY_UCE		((u64)MEMATTR_UCE)	/* uncached, exported */
#define EFI_MEMORY_WP		((u64)MEMATTR_WP)	/* write-protect */
#define EFI_MEMORY_RP		((u64)MEMATTR_RP)	/* read-protect */
#define EFI_MEMORY_XP		((u64)MEMATTR_XP)	/* execute-protect */
#define EFI_MEMORY_NV		((u64)MEMATTR_NV)	/* non-volatile */
#define EFI_MEMORY_RUNTIME	((u64)0x8000000000000000)	/* range requires runtime mapping */

#define EFI_MEMORY_MORE_RELIABLE ((u64)MEMATTR_MORE_RELIABLE)	/* higher reliability */
#define EFI_MEMORY_RO		((u64)MEMATTR_RO)	/* read-only */
#define EFI_MEMORY_SP		((u64)MEMATTR_SP)	/* specific-purpose memory (SPM) */

static inline enum efi_memory_type resource_type_to_efi_memory_type(unsigned type)
{
	return type;
}

static inline unsigned efi_memory_type_to_resource_type(enum efi_memory_type type)
{
	return type;
}

static inline enum efi_memory_type resource_get_efi_memory_type(const struct resource *res)
{
	if (res->flags & IORESOURCE_TYPE_VALID)
		return resource_type_to_efi_memory_type(res->type);
	return EFI_CONVENTIONAL_MEMORY;
}

static inline u64 resource_get_efi_memory_attrs(const struct resource *res)
{
	if (res->flags & IORESOURCE_TYPE_VALID)
		return res->attrs | (res->runtime ? EFI_MEMORY_RUNTIME : 0);
	return 0;
}

static inline void resource_set_efi_memory_type_attrs(struct resource *res,
						enum efi_memory_type type,
						u64 attrs)
{
	res->type = type;
	res->runtime = attrs & EFI_MEMORY_RUNTIME;
	res->attrs = attrs & ~EFI_MEMORY_RUNTIME;
	res->flags |= IORESOURCE_TYPE_VALID;
}

#endif
