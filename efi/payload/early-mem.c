// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <linux/sizes.h>
#include <efi.h>
#include <efi/efi-payload.h>

efi_physical_addr_t efi_earlymem_alloc(const struct efi_system_table *sys_table,
				       size_t *memsize)
{
	struct efi_boot_services *bs = sys_table->boottime;
	efi_physical_addr_t mem;
	efi_status_t efiret;

	mem = IS_ENABLED(CONFIG_X86) ? 0x3fffffff : ~0ULL;
	for (*memsize = SZ_256M; *memsize >= SZ_8M; *memsize /= 2) {
		efiret  = bs->allocate_pages(EFI_ALLOCATE_MAX_ADDRESS,
					     EFI_LOADER_DATA,
					     *memsize / EFI_PAGE_SIZE, &mem);
		if (!EFI_ERROR(efiret) || efiret != EFI_OUT_OF_RESOURCES)
			break;
	}
	if (EFI_ERROR(efiret))
		panic("failed to allocate %zu byte memory pool: 0x%lx\n",
		      *memsize, efiret);

	return mem;
}
