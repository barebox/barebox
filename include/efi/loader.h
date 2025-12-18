/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-SnippetCopyrightText: 2016 Alexander Graf */

#ifndef _EFI_LOADER_H
#define _EFI_LOADER_H 1

#include <efi/types.h>
#include <efi/services.h>
#include <efi/memory.h>

#define EFI_SPECIFICATION_VERSION (2 << 16 | 80)

/* Key identifying current memory map */
extern efi_uintn_t efi_memory_map_key;

/* Allocate boot service data pool memory */
void *efi_alloc(size_t len, const char *name);
/* Reallocate boot service data pool memory */
efi_status_t efi_realloc(void **ptr, size_t len, const char *name);
/* Allocate pages on the specified alignment */
void *efi_alloc_aligned_pages(u64 len, int memory_type, size_t align,
			      const char *name);
/* More specific EFI memory allocator, called by EFI payloads */
efi_status_t efi_allocate_pages(enum efi_allocate_type type,
				enum efi_memory_type memory_type,
				efi_uintn_t pages, uint64_t *memory,
				const char *name);
/* EFI memory free function. */
efi_status_t efi_free_pages(uint64_t memory, efi_uintn_t pages);
/* EFI memory allocator for small allocations */
efi_status_t efi_allocate_pool(enum efi_memory_type pool_type,
			       efi_uintn_t size, void **buffer,
			       const char *name);
/* EFI pool memory free function. */
efi_status_t efi_free_pool(void *buffer);
/* Returns the EFI memory map */
efi_status_t efi_get_memory_map(efi_uintn_t *memory_map_size,
				struct efi_memory_desc *memory_map,
				efi_uintn_t *map_key,
				efi_uintn_t *descriptor_size,
				uint32_t *descriptor_version);

#endif /* _EFI_LOADER_H */
