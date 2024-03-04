// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 Ahmad Fatoum, Pengutronix

#define pr_fmt(fmt) "efi-iomem: " fmt

#include <common.h>
#include <init.h>
#include <efi.h>
#include <efi/efi-payload.h>
#include <memory.h>
#include <linux/sizes.h>

static int efi_parse_mmap(struct efi_memory_desc *desc, bool verbose)
{
	struct resource *res;
	u32 flags;
	const char *name;
	char *fullname;
	resource_size_t va_base, va_size;
	int ret = 0;

	va_size = desc->npages * SZ_4K;
	if (!va_size)
		return 0;

	/* XXX At least OVMF doesn't populate ->virt_start and leaves it at zero
	 * for all mapping. Thus assume a 1:1 mapping and ignore virt_start
	 */
	va_base = desc->phys_start;

	switch (desc->type) {
	case EFI_RESERVED_TYPE:
		if (verbose)
			return 0;
		name = "reserved";
		flags = IORESOURCE_MEM | IORESOURCE_DISABLED;
		break;
	case EFI_LOADER_CODE:
		name = "loader code";
		flags = IORESOURCE_MEM | IORESOURCE_READONLY;
		break;
	case EFI_LOADER_DATA:
		name = "loader data";
		flags = IORESOURCE_MEM;
		break;
	case EFI_BOOT_SERVICES_CODE:
		if (!verbose)
			return 0;
		name = "boot services code";
		flags = IORESOURCE_MEM | IORESOURCE_READONLY;
		break;
	case EFI_BOOT_SERVICES_DATA:
		if (!verbose)
			return 0;
		name = "boot services data";
		flags = IORESOURCE_MEM;
		break;
	case EFI_RUNTIME_SERVICES_CODE:
		if (!verbose)
			return 0;
		name = "runtime services code";
		flags = IORESOURCE_MEM | IORESOURCE_READONLY;
		break;
	case EFI_RUNTIME_SERVICES_DATA:
		if (!verbose)
			return 0;
		name = "runtime services data";
		flags = IORESOURCE_MEM;
		break;
	case EFI_CONVENTIONAL_MEMORY:
		if (!verbose)
			return 0;
		name = "conventional memory";
		flags = IORESOURCE_MEM | IORESOURCE_PREFETCH | IORESOURCE_CACHEABLE;
		break;
	case EFI_UNUSABLE_MEMORY:
		if (!verbose)
			return 0;
		name = "unusable";
		flags = IORESOURCE_MEM | IORESOURCE_DISABLED;
		break;
	case EFI_ACPI_RECLAIM_MEMORY:
		if (!verbose)
			return 0;
		name = "ACPI reclaim memory";
		flags = IORESOURCE_MEM | IORESOURCE_READONLY;
		break;
	case EFI_ACPI_MEMORY_NVS:
		if (!verbose)
			return 0;
		name = "ACPI NVS memory";
		flags = IORESOURCE_MEM | IORESOURCE_READONLY;
		break;
	case EFI_MEMORY_MAPPED_IO:
		if (!verbose)
			return 0;
		name = "MMIO";
		flags = IORESOURCE_MEM;
		break;
	case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
		if (!verbose)
			return 0;
		name = "MMIOPORT";
		flags = IORESOURCE_IO;
		break;
	case EFI_PAL_CODE:
		if (!verbose)
			return 0;
		name = "PAL code";
		flags = IORESOURCE_MEM | IORESOURCE_ROM_BIOS_COPY;
		break;
	default:
		if (!(desc->type & (1U << 31))) {
			pr_warn("illegal memory type = %u >= %u\n",
				desc->type, EFI_MAX_MEMORY_TYPE);
			return -EINVAL;
		}

		if (!verbose)
			return 0;

		name = "vendor reserved";
		flags = IORESOURCE_MEM | IORESOURCE_ROM_BIOS_COPY;
	}

	fullname = xasprintf("%s@%llx", name, desc->phys_start);

	pr_debug("%s: (0x%llx+0x%llx)\n", fullname, va_base, va_size);

	res = request_iomem_region(fullname, va_base, va_base + va_size - 1);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
		goto out;
	}

	res->flags |= flags;

out:
	free(fullname);
	return ret;
}

static int efi_barebox_populate_mmap(void)
{
	void *mmap_buf = NULL, *desc;
	efi_status_t efiret;
	size_t mmap_size;
	size_t mapkey;
	size_t descsz;
	u32 descver;
	int ret = 0;

	mmap_size = sizeof(struct efi_memory_desc);

	do {
		mmap_buf = xrealloc(mmap_buf, mmap_size);
		efiret = BS->get_memory_map(&mmap_size, mmap_buf,
					    &mapkey, &descsz, &descver);
	} while (efiret == EFI_BUFFER_TOO_SMALL);

	if (EFI_ERROR(efiret)) {
		ret = -efi_errno(efiret);
		goto out;
	}

	if (descver != 1) {
		ret = -ENOSYS;
		goto out;
	}

	for (desc = mmap_buf; desc < mmap_buf + mmap_size; desc += descsz)
		efi_parse_mmap(desc, __is_defined(DEBUG));

out:
	free(mmap_buf);
	return ret;
}
mem_initcall(efi_barebox_populate_mmap);
