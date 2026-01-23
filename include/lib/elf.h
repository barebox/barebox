/* SPDX-License-Identifier: GPL-2.0-only */
/* ELF utility functions that require no allocations */
#ifndef _LIB_ELF_H
#define _LIB_ELF_H

#include <linux/types.h>
#include <linux/list.h>

struct elf_image {
	struct list_head list;
	u8 class;
	u16 type;		/* ET_EXEC or ET_DYN */
	u64 entry;
	void *low_addr;
	void *high_addr;
	void *hdr_buf;
	const char *filename;
	void *load_address;	/* User-specified load address (NULL = use p_paddr) */
	void *base_load_addr;	/* Calculated base address for ET_DYN */
	unsigned long reloc_offset;	/* Offset between p_vaddr and actual load address */
};

static inline void elf_init_struct(struct elf_image *elf)
{
	INIT_LIST_HEAD(&elf->list);
	elf->low_addr = (void *) (unsigned long) -1;
	elf->high_addr = 0;
	elf->filename = NULL;
}

int elf_check_image(struct elf_image *elf, void *buf);
int elf_open_binary_into(struct elf_image *elf, void *buf);

/*
 * Set the load address for the ELF file.
 * Must be called before elf_load().
 * If not set, ET_EXEC uses p_paddr, ET_DYN uses lowest p_paddr.
 */
void elf_set_load_address(struct elf_image *elf, void *addr);

/*
 * Parse the dynamic section and extract relocation information.
 * This is a generic function that works for both 32-bit and 64-bit ELF files,
 * and handles both REL and RELA relocation formats.
 * Returns 0 on success, -EINVAL on error.
 */
void *elf_find_dynamic_segment(struct elf_image *elf);

/*
 * Parse the dynamic section and extract relocation information.
 * This is a generic function that works for both 32-bit and 64-bit ELF files,
 * and handles both REL and RELA relocation formats.
 * Returns 0 on success, -EINVAL on error.
 */
int elf_parse_dynamic_section_rel(struct elf_image *elf, const void *dyn_seg,
				  void **rel_out, u64 *relsz_out, void **symtab);
int elf_parse_dynamic_section_rela(struct elf_image *elf, const void *dyn_seg,
				   void **rel_out, u64 *relsz_out, void **symtab);

/*
 * Apply dynamic relocations to an ELF binary already loaded in memory.
 * This modifies the ELF image in place without allocating new memory.
 * Useful for self-relocating loaders or externally loaded binaries.
 * The elf parameter must have been previously opened with elf_open_binary().
 */
int elf_load_inplace(struct elf_image *elf);

/*
 * Architecture-specific relocation handler.
 * Returns 0 on success, -ENOSYS if architecture doesn't support relocations,
 * other negative error codes on failure.
 */
int elf_apply_relocations(struct elf_image *elf, const void *dyn_seg);

#endif
