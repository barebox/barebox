// SPDX-License-Identifier: GPL-2.0+

#define pr_fmt(fmt) "efi-loader: memory: " fmt

#include <linux/minmax.h>
#include <linux/printk.h>
#include <linux/sprintf.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <init.h>
#include <memory.h>
#include <linux/list_sort.h>
#include <linux/sizes.h>
#include <dma.h>

efi_uintn_t efi_memory_map_key;

static efi_status_t find_pages_max(struct list_head *banks, size_t npages, size_t *page)
{
	struct memory_bank *bank;

	list_for_each_entry_reverse(bank, banks, list) {
		if ((bank->res->start >> EFI_PAGE_SHIFT) > *page)
			continue;

		for_each_memory_bank_region_reverse(bank, region) {
			resource_size_t gap_firstpage, gap_lastpage;
			resource_size_t candidate_last;

			if (!region_is_gap(region))
				continue;

			gap_firstpage = EFI_PAGE_ALIGN(region->start) >> EFI_PAGE_SHIFT;
			gap_lastpage = (EFI_PAGE_ALIGN(region->end + 1) >> EFI_PAGE_SHIFT);

			if (!gap_lastpage || gap_lastpage < gap_firstpage)
				continue;

			/* make last page inclusive */
			gap_lastpage--;

			candidate_last = min_t(resource_size_t, gap_lastpage, *page);
			if (candidate_last < gap_firstpage)
				continue;

			if (candidate_last - gap_firstpage + 1 < npages)
				continue;

			*page = candidate_last + 1 - npages;
			return EFI_SUCCESS;
		}
	}

	return EFI_OUT_OF_RESOURCES;
}


static const char *efi_memory_type_tostr(enum efi_memory_type type)
{
	switch (type) {
	case EFI_RESERVED_TYPE:
		return "res";
	case EFI_LOADER_CODE:
		return "ldcode";
	case EFI_LOADER_DATA:
		return "lddata";
	case EFI_BOOT_SERVICES_CODE:
		return "bscode";
	case EFI_BOOT_SERVICES_DATA:
		return "bsdata";
	case EFI_RUNTIME_SERVICES_CODE:
		return "rtcode";
	case EFI_RUNTIME_SERVICES_DATA:
		return "rtdata";
	case EFI_CONVENTIONAL_MEMORY:
		return "memory";
	case EFI_UNUSABLE_MEMORY:
		return "unsable";
	case EFI_ACPI_RECLAIM_MEMORY:
		return "acpireclaim";
	case EFI_ACPI_MEMORY_NVS:
		return "acpinvs";
	case EFI_MEMORY_MAPPED_IO:
		return "mmio";
	case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
		return "mmioport";
	case EFI_PAL_CODE:
		return "pal";
	case EFI_PERSISTENT_MEMORY_TYPE:
		return "persistent";
	case EFI_UNACCEPTED_MEMORY_TYPE:
		return "unaccepted";
	default:
		return "unknown";
	}
}

static u64 efi_memory_type_default_attrs(enum efi_memory_type type)
{
	switch (type) {
	case EFI_RESERVED_TYPE:
	case EFI_MEMORY_MAPPED_IO:
	case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
	case EFI_UNACCEPTED_MEMORY_TYPE:
		return 0;
	case EFI_BOOT_SERVICES_CODE:
	case EFI_LOADER_CODE:
	case EFI_RUNTIME_SERVICES_CODE:
	case EFI_ACPI_RECLAIM_MEMORY:
	case EFI_ACPI_MEMORY_NVS:
		return MEMATTRS_RWX;
	case EFI_LOADER_DATA:
	case EFI_BOOT_SERVICES_DATA:
	case EFI_RUNTIME_SERVICES_DATA:
		return MEMATTRS_RW;
	case EFI_CONVENTIONAL_MEMORY:
		return MEMATTRS_RWX;
	case EFI_PAL_CODE:
		return MEMATTRS_FAULT;
	case EFI_PERSISTENT_MEMORY_TYPE:
		return MEMATTRS_RW | MEMATTR_SP;
	case EFI_UNUSABLE_MEMORY:
	case EFI_MAX_MEMORY_TYPE:
		pr_warn("Unallocatable type %u\n", type); // FIXME
		return MEMATTRS_FAULT;
	}

	return MEMATTRS_RWX;
}

/**
 * efi_allocate_pages - allocate memory pages
 *
 * @type:		type of allocation to be performed
 * @memory_type:	usage type of the allocated memory
 * @pages:		number of pages to be allocated
 * @memory:		allocated memory
 * @name:		name for informational purposes
 * Return:		status code
 */
efi_status_t efi_allocate_pages(enum efi_allocate_type type,
				enum efi_memory_type memory_type,
				efi_uintn_t npages, uint64_t *memory,
				const char *name)
{
	char namebuf[64];
	const char *typestr;
	struct resource *res;
	uint64_t attrs, new_addr = *memory;
	size_t new_page;
	efi_status_t r;

	++efi_memory_map_key;

	if (npages == 0)
		return EFI_INVALID_PARAMETER;

	switch (type) {
	case EFI_ALLOCATE_ANY_PAGES:
		new_addr = U64_MAX;
		typestr = "any";
		fallthrough;
	case EFI_ALLOCATE_MAX_ADDRESS:
		new_page = new_addr >> EFI_PAGE_SHIFT;
		r = find_pages_max(&memory_banks, npages, &new_page);
		if (r != EFI_SUCCESS)
			return r;
		new_addr = new_page << EFI_PAGE_SHIFT;
		typestr = "max";
		break;
	case EFI_ALLOCATE_ADDRESS:
		typestr = "exact";
		break;
	default:
		/* UEFI doesn't specify other allocation types */
		return EFI_INVALID_PARAMETER;
	}

	scnprintf(namebuf, sizeof(namebuf), "efi%zu-%s%c%s", efi_memory_map_key,
		  efi_memory_type_tostr(memory_type),
		  name ? '-' : '\0', name ?: "");

	attrs = efi_memory_type_default_attrs(memory_type);
	if (!attrs)
		return EFI_INVALID_PARAMETER;

	res = request_sdram_region(namebuf, new_addr, npages << EFI_PAGE_SHIFT,
				   efi_memory_type_to_resource_type(memory_type),
				   attrs);
	if (!res) {
		pr_err("failed to request %s at page 0x%zx+%zu (%s)\n",
		       namebuf, new_page, npages, typestr);
		dump_stack();
		return EFI_OUT_OF_RESOURCES;
	}

	if (memory_type == EFI_RUNTIME_SERVICES_CODE ||
	    memory_type == EFI_RUNTIME_SERVICES_DATA)
		res->runtime = true;

	res->flags |= IORESOURCE_EFI_ALLOC;

	*memory = new_addr;
	return EFI_SUCCESS;
}

static int free_efi_only(struct resource *res, void *data)
{
	int *nfreed = data;

	if (!(res->flags & IORESOURCE_EFI_ALLOC)) {
		pr_warn("refusing to free non-EFI allocated resource %s at 0x%llx\n",
			res->name, res->start);
		*nfreed = -1;
		return false;
	}

	if (nfreed >= 0)
		++*nfreed;
	return true;
}

// SPDX-SnippetBegin
// SPDX-License-Identifier: GPL-2.0+
// SPDX-SnippetCopyrightText: 2016 Alexander Graf
// SPDX-SnippetComment: Origin-URL: https://github.com/u-boot/u-boot/blob/aa703a816a62deb876a1e77ccff030a7cc60f344/lib/efi_loader/efi_memory.c

/**
 * efi_alloc_aligned_pages() - allocate aligned memory pages
 *
 * @len:		len in bytes
 * @memory_type:	usage type of the allocated memory
 * @align:		alignment in bytes
 * @name:		name for informational purposes
 * Return:		aligned memory or NULL
 */
void *efi_alloc_aligned_pages(u64 len, int memory_type, size_t align,
			      const char *name)
{
	u64 req_pages = efi_size_in_pages(len);
	u64 true_pages = req_pages + efi_size_in_pages(align) - 1;
	u64 free_pages;
	u64 aligned_mem;
	efi_status_t r;
	u64 mem;

	/* align must be zero or a power of two */
	if (align & (align - 1))
		return NULL;

	/* Check for overflow */
	if (true_pages < req_pages)
		return NULL;

	if (align < EFI_PAGE_SIZE) {
		r = efi_allocate_pages(EFI_ALLOCATE_ANY_PAGES, memory_type,
				       req_pages, &mem, name);
		return (r == EFI_SUCCESS) ? (void *)(uintptr_t)mem : NULL;
	}

	r = efi_allocate_pages(EFI_ALLOCATE_ANY_PAGES, memory_type,
			       true_pages, &mem, name);
	if (r != EFI_SUCCESS)
		return NULL;

	aligned_mem = ALIGN(mem, align);
	/* Free pages before alignment */
	free_pages = efi_size_in_pages(aligned_mem - mem);
	if (free_pages)
		efi_free_pages(mem, free_pages);

	/* Free trailing pages */
	free_pages = true_pages - (req_pages + free_pages);
	if (free_pages) {
		mem = aligned_mem + req_pages * EFI_PAGE_SIZE;
		efi_free_pages(mem, free_pages);
	}

	return (void *)(uintptr_t)aligned_mem;
}

// SPDX-SnippetEnd

/**
 * efi_free_pages() - free memory pages
 *
 * @memory:	start of the memory area to be freed
 * @pages:	number of pages to be freed
 * Return:	status code
 */
efi_status_t efi_free_pages(uint64_t memory, size_t pages)
{
	size_t size = pages << EFI_PAGE_SHIFT;
	struct memory_bank *bank;
	int nfreed = 0;

	for_each_memory_bank(bank)
		release_region_range(bank->res, memory, size, free_efi_only, &nfreed);

	if (!nfreed)
		pr_warn("can't free %llx: not found\n", memory);
	if (nfreed <= 0)
		return EFI_INVALID_PARAMETER;

	return EFI_SUCCESS;
}

static int efi_memory_desc_from_res(const struct resource *region,
				     struct efi_memory_desc *desc)
{
	efi_physical_addr_t phys_start = EFI_PAGE_ALIGN(region->start);
	efi_uintn_t npages = efi_size_in_pages(EFI_PAGE_ALIGN(region->end + 1) - phys_start);

	if (!npages)
		return 0;

	desc->phys_start = phys_start;
	desc->virt_start = efi_phys_to_virt(phys_start);
	desc->npages = npages;

	if (region->flags & IORESOURCE_TYPE_VALID) {
		desc->type = resource_get_efi_memory_type(region);
		desc->attrs = resource_get_efi_memory_attrs(region);
	} else {
		pr_warn("encountered SDRAM region 0x%pa-0x%pa without valid type\n",
			&region->start, &region->end);
		desc->type = EFI_RESERVED_TYPE;
		desc->attrs = EFI_MEMORY_WB;
	}

	return 1;
}

/**
 * efi_get_memory_map() - get map describing memory usage.
 *
 * @memory_map_size:	on entry the size, in bytes, of the memory map buffer,
 *			on exit the size of the copied memory map
 * @memory_map:		buffer to which the memory map is written
 * @map_key:		key for the memory map
 * @descriptor_size:	size of an individual memory descriptor
 * @descriptor_version:	version number of the memory descriptor structure
 * Return:		status code
 */
efi_status_t efi_get_memory_map(size_t *memory_map_size,
			       struct efi_memory_desc *memory_map,
			       efi_uintn_t *map_key,
			       size_t *descriptor_size,
			       uint32_t *descriptor_version)
{
	size_t map_size = 0;
	int i = 0, map_entries = 0;
	size_t provided_map_size;
	struct memory_bank *bank;

	if (!memory_map_size)
		return EFI_INVALID_PARAMETER;

	provided_map_size = *memory_map_size;

	for_each_memory_bank(bank) {
		for_each_memory_bank_region(bank, region) {
			if (list_empty(&region->children)) {
				map_entries++;
				continue;
			}

			for_each_resource_region(region, bbregion)
				map_entries++;
		}
	}

	map_size = map_entries * sizeof(struct efi_memory_desc);

	/* Note: some regions may end up being 0-sized after alignment to page
	 * boundaries and those will be skipped later.
	 *
	 * It's fine wrt *memory_map_size though as worst case, this means we
	 * ask the caller to allocate a little more memory than actually needed.
	 *
	 * TODO: Should we rather enforce resource allocations from memory banks
	 * to be page aligned from the outset?
	 */
	*memory_map_size = map_size;

	if (descriptor_size)
		*descriptor_size = sizeof(struct efi_memory_desc);

	if (descriptor_version)
		*descriptor_version = EFI_MEMORY_DESCRIPTOR_VERSION;

	if (provided_map_size < map_size)
		return EFI_BUFFER_TOO_SMALL;

	if (!memory_map)
		return EFI_INVALID_PARAMETER;

	for_each_memory_bank(bank) {
		for_each_memory_bank_region(bank, region) {
			if (list_empty(&region->children)) {
				i += efi_memory_desc_from_res(region, &memory_map[i]);
				continue;
			}

			for_each_resource_region(region, bbregion)
				i += efi_memory_desc_from_res(bbregion, &memory_map[i]);
		}
	}

	*memory_map_size = i * sizeof(struct efi_memory_desc);

	if (map_key)
		*map_key = efi_memory_map_key;

	return EFI_SUCCESS;
}
