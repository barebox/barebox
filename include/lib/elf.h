/* SPDX-License-Identifier: GPL-2.0-only */
/* ELF utility functions that require no allocations */
#ifndef _LIB_ELF_H
#define _LIB_ELF_H

#include <linux/types.h>
#include <linux/list.h>

struct elf_image {
	u8 class;
	u16 type;		/* ET_EXEC or ET_DYN */
	u64 entry;
	void *hdr_buf;
	void *load_address;	/* User-specified load address (NULL = use p_paddr) */
	unsigned long reloc_offset;	/* Offset between p_vaddr and actual load address */

	/* Only used in barebox proper */
	struct list_head list;
	void *low_addr;
	void *high_addr;
	const char *filename;
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
 * Find the first segment of the given p_type and return its address,
 * relocated if the image was loaded with elf_load_inplace(). If @memsz
 * is not NULL, it's set to the segment's p_memsz.
 * Returns NULL if there is no such segment.
 */
void *elf_find_segment(struct elf_image *elf, u32 type, u64 *memsz);

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
int elf_parse_dynamic_section_relr(struct elf_image *elf, const void *dyn_seg,
				   void **relr_out, u64 *relrsz_out);

/*
 * Return the memory an image occupies from its start when loaded with
 * elf_load_inplace(), i.e. the maximum of p_offset + p_memsz over all
 * PT_LOAD segments.
 */
u64 elf_inplace_size(struct elf_image *elf);

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
