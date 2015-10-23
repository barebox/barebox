/*
 * Copyright 2014 GE Intelligent Platforms, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/cache.h>
#include <mmu.h>
#include <mach/mmu.h>

int arch_remap_range(void *_start, size_t size, unsigned flags)
{
	uint32_t ptr, start, tsize, valid, wimge, pte_flags;
	unsigned long epn;
	phys_addr_t rpn = 0;
	int esel = 0;

	switch (flags) {
	case MAP_UNCACHED:
		pte_flags = MAS2_I;
		break;
	case MAP_CACHED:
		pte_flags = 0;
		break;
	default:
		return -EINVAL;
	}

	ptr = start = (uint32_t)_start;
	wimge = pte_flags | MAS2_M;

	while (ptr < (start + size)) {
		esel = e500_find_tlb_idx((void *)ptr, 1);
		if (esel != -1)
			break;
		e500_read_tlbcam_entry(esel, &valid, &tsize, &epn,
				&rpn);
		if (flags & MAS2_I) {
			flush_dcache();
			invalidate_icache();
		}
		e500_set_tlb(1, epn, rpn, MAS3_SX|MAS3_SW|MAS3_SR,
				(u8)wimge, 0, esel, tsize, 1);
		/* convert tsize to bytes to increment address. */
		ptr += (1ULL << ((tsize) + 10));
	}

	return 0;
}
