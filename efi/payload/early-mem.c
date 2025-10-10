// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <linux/sizes.h>
#include <efi.h>
#include <efi/efi-payload.h>

void *efi_earlymem_alloc(const struct efi_system_table *sys_table,
			 size_t memsize, enum efi_memory_type mem_type)
{
	struct efi_boot_services *bs = sys_table->boottime;
	efi_physical_addr_t mem;
	efi_status_t efiret;

	efiret  = bs->allocate_pages(EFI_ALLOCATE_ANY_PAGES, mem_type,
					     memsize / EFI_PAGE_SIZE, &mem);
	if (EFI_ERROR(efiret))
		panic("failed to allocate %zu byte memory pool: 0x%lx\n",
		      memsize, efiret);

	return efi_phys_to_virt(mem);
}
