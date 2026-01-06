// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <linux/compiler.h>
#include <string.h>
#include <barebox.h>
#include <elf.h>
#include <debug_ll.h>
#include <asm/reloc.h>

#define R_ARM_RELATIVE 23

/*
 * relocate binary to the currently running address
 */
void __prereloc relocate_image(unsigned long offset,
			       void *dstart, void *dend,
			       long *dynsym, long *dynend)
{
	while (dstart < dend) {
		struct elf32_rel *rel = dstart;
		unsigned long r, *fixup;

		switch (ELF32_R_TYPE(rel->r_info)) {
		case R_ARM_RELATIVE:
			fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = *fixup + offset;

			rel->r_offset += offset;
			break;
		case R_ARM_ABS32:
			r = dynsym[ELF32_R_SYM(rel->r_info) * 4 + 1];
			fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = *fixup + r + offset;
			rel->r_offset += offset;
			break;
		case R_ARM_NONE:
			break;
		default:
			putc_ll('>');
			puthex_ll(rel->r_info);
			putc_ll(' ');
			puthex_ll(rel->r_offset);
			putc_ll('\n');
			__hang();
		}

		dstart += sizeof(*rel);
	}

	/* Optional: not required for correctness */
	if (dynend)
		__memset(dynsym, 0, (unsigned long)dynend - (unsigned long)dynsym);
}
