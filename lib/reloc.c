// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/arch/arm64/kernel/pi/relocate.c?id=97a6f43bb049e64b9913c50c7530e13d78e205d4
// SPDX-FileCopyrightText: 2023 Google LLC
// Authors: Ard Biesheuvel <ardb@google.com>
//          Peter Collingbourne <pcc@google.com>

#include <asm-generic/reloc.h>
#include <linux/elf.h>

/**
 * relocate_relr - Apply RELR relocations.
 * @offset: offset to relocate barebox by
 * @relr_start: first RELR relocation entry
 * @relr_end: (exclusive) end address just after the last RELR rentry
 *
 * RELR is a compressed format for storing relative relocations. The
 * encoded sequence of entries looks like:
 * [ AAAAAAAA BBBBBBB1 BBBBBBB1 ... AAAAAAAA BBBBBB1 ... ]
 *
 * i.e. start with an address, followed by any number of bitmaps. The
 * address entry encodes 1 relocation. The subsequent bitmap entries
 * encode up to 63 relocations each, at subsequent offsets following
 * the last address entry.
 *
 * The bitmap entries must have 1 in the least significant bit. The
 * assumption here is that an address cannot have 1 in lsb. Odd
 * addresses are not supported. Any odd addresses are stored in the
 * REL/RELA section, which is handled separately.
 *
 * With the exception of the least significant bit, each bit in the
 * bitmap corresponds with a machine word that follows the base address
 * word, and the bit value indicates whether or not a relocation needs
 * to be applied to it. The second least significant bit represents the
 * machine word immediately following the initial address, and each bit
 * that follows represents the next word, in linear order. As such, a
 * single bitmap can encode up to 63 relocations in a 64-bit object.
 */
void __prereloc
relocate_relr(unsigned long offset, void *relr_start, void *relr_end)
{
	elf_relr_t *place = NULL;

	if (!IS_ENABLED(CONFIG_RELR) || !offset)
		return;

	for (elf_relr_t *relr = relr_start;
	     (void *)relr < relr_end; relr++) {
		if ((*relr & 1) == 0) {
			place = (elf_relr_t *)(*relr + offset);
			*place++ += offset;

                       /*
                        * Rebase the entry so it tracks the effective link
                        * address, like RELA's r_offset += offset. This
                        * allows relocate_to_current_adr() to be called
                        * again after the binary is copied to a new address
                        * (e.g. relocate_to_adr_full or PBL copying itself
                        * into DRAM on i.MX8MM verified boot).
                        */
                       *relr += offset;
		} else {
			for (elf_relr_t *p = place, r = *relr >> 1; r; p++, r >>= 1)
				if (r & 1)
					*p += offset;
			place += ELF_RELR_BITMAP_BITS;
		}
	}
}
