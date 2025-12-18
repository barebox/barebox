/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-SnippetCopyrightText: 2016 Alexander Graf */

#ifndef _EFI_LOADER_H
#define _EFI_LOADER_H 1

#include <efi/types.h>
#include <efi/services.h>
#include <efi/memory.h>

struct efi_table_hdr;

#define EFI_SPECIFICATION_VERSION (2 << 16 | 80)

/* GUID used by the root node. */
#define BAREBOX_GUID \
	EFI_GUID(0x0f38fd98, 0x6788, 0x5379, 0x9b, 0xae, 0x43, 0x05, 0xb3, 0x58, 0x4f, 0x82)

/* Key identifying current memory map */
extern efi_uintn_t efi_memory_map_key;

extern struct efi_runtime_services efi_runtime_services;
extern struct efi_system_table systab;

/* Called by bootefi to initialize runtime */
efi_status_t efi_alloc_system_table(void);
efi_status_t efi_initialize_system_table(void);

/* Called by efi_set_watchdog_timer to reset the timer */
efi_status_t efi_set_watchdog(unsigned long timeout);

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

/* Indicate supported runtime services */
efi_status_t efi_init_runtime_supported(void);

/* Update CRC32 in table header */
void efi_update_table_header_crc32(struct efi_table_hdr *table);

/* replacement function, returns EFI_UNSUPPORTED */
efi_status_t __efi_runtime EFIAPI efi_unimplemented(void);

enum efi_loader_state;

void efi_loader_set_state(enum efi_loader_state);

extern struct device efidev;

void efi_register_deferred_init(efi_status_t (*init)(void *), void *);
void efi_add_root_node_protocol_deferred(const efi_guid_t *protocol, const void *interface);

int efiloader_esp_mount_dir(void);

#endif /* _EFI_LOADER_H */
