// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "pbl-mmu: " fmt

#include <common.h>
#include <elf.h>
#include <mmu.h>
#include <pbl/mmu.h>
#include <asm/mmu.h>
#include <linux/bits.h>
#include <linux/sizes.h>

/*
 * Map ELF segment permissions (p_flags) to architecture MMU flags
 */
static unsigned int elf_flags_to_mmu_flags(u32 p_flags)
{
	bool readable = p_flags & PF_R;
	bool writable = p_flags & PF_W;
	bool executable = p_flags & PF_X;

	if (readable && writable) {
		/* Data, BSS: Read-write, cached, non-executable */
		return MAP_CACHED;
	} else if (readable && executable) {
		/* Text: Read-only, cached, executable */
		return MAP_CODE;
	} else if (readable) {
		/* Read-only data: Read-only, cached, non-executable */
		return MAP_CACHED_RO;
	} else {
		/*
		 * Unusual: segment with no read permission.
		 * Map as uncached, non-executable for safety.
		 */
		pr_warn("Segment with unusual permissions: flags=0x%x\n", p_flags);
		return MAP_UNCACHED;
	}
}

int pbl_mmu_setup_from_elf(struct elf_image *elf, unsigned long membase,
			    unsigned long memsize)
{
	void *phdr;
	int i = -1;

	pr_debug("Setting up MMU from ELF segments\n");
	pr_debug("ELF loaded at: 0x%p - 0x%p\n", elf->low_addr, elf->high_addr);

	/*
	 * Iterate through all PT_LOAD segments and set up MMU permissions
	 * based on the segment's p_flags
	 */
	elf_for_each_segment(phdr, elf, elf->hdr_buf) {
		i++;

		if (elf_phdr_p_type(elf, phdr) != PT_LOAD)
			continue;

		u64 p_vaddr = elf_phdr_p_vaddr(elf, phdr);
		u64 p_memsz = elf_phdr_p_memsz(elf, phdr);
		u32 p_flags = elf_phdr_p_flags(elf, phdr);

		/*
		 * Calculate actual address after relocation.
		 * For ET_EXEC: reloc_offset is 0, use p_vaddr directly
		 * For ET_DYN: reloc_offset adjusts virtual to actual address
		 */
		unsigned long addr = p_vaddr + elf->reloc_offset;
		unsigned long size = p_memsz;
		unsigned long segment_end = addr + size;

		/* Validate segment is within available memory */
		if (segment_end < addr || /* overflow check */
		    addr < membase ||
		    segment_end > membase + memsize) {
			pr_err("Segment %d outside memory bounds\n", i);
			return -EINVAL;
		}

		/* Validate alignment - warn and round if needed */
		if (!IS_ALIGNED(size, PAGE_SIZE)) {
			pr_debug("Segment %d not page-aligned, rounding\n", i);
			size = ALIGN(size, PAGE_SIZE);
		}

		unsigned int mmu_flags = elf_flags_to_mmu_flags(p_flags);

		pr_debug("Segment %d: addr=0x%08lx size=0x%08lx flags=0x%x [%c%c%c] -> mmu_flags=0x%x\n",
			 i, addr, size, p_flags,
			 (p_flags & PF_R) ? 'R' : '-',
			 (p_flags & PF_W) ? 'W' : '-',
			 (p_flags & PF_X) ? 'X' : '-',
			 mmu_flags);

		/*
		 * Remap this segment with proper permissions.
		 */
		remap_range((void *)addr, size, mmu_flags);
	}

	pr_debug("MMU setup from ELF complete\n");
	return 0;
}
