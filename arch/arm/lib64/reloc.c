// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <linux/compiler.h>
#include <string.h>
#include <barebox.h>
#include <elf.h>
#include <debug_ll.h>
#include <asm/reloc.h>

/*
 * relocate binary to the currently running address
 */
void __prereloc relocate_image(unsigned long offset,
			       void *dstart, void *dend,
			       long *dynsym, long *dynend)
{
	while (dstart < dend) {
		struct elf64_rela *rel = dstart;
		unsigned long *fixup;

		switch(ELF64_R_TYPE(rel->r_info)) {
		case R_AARCH64_RELATIVE:
			fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = rel->r_addend + offset;
			rel->r_addend += offset;
			rel->r_offset += offset;
			break;
		case R_ARM_NONE:
			break;
		default:
			putc_ll('>');
			puthex_ll(rel->r_info);
			putc_ll(' ');
			puthex_ll(rel->r_offset);
			putc_ll(' ');
			puthex_ll(rel->r_addend);
			putc_ll('\n');
			__hang();
		}

		dstart += sizeof(*rel);
	}
}

/*
 * Apply ARM64 ELF relocations
 */
int elf_apply_relocations(struct elf_image *elf, const void *dyn_seg)
{
	void *rela_ptr = NULL, *symtab = NULL;
	u64 relasz;
	phys_addr_t base = (phys_addr_t)elf->reloc_offset;
	int ret;

	ret = elf_parse_dynamic_section_rela(elf, dyn_seg, &rela_ptr, &relasz, &symtab);
	if (ret)
		return ret;

	relocate_image(base, rela_ptr, rela_ptr + relasz, symtab, NULL);

	return 0;
}
