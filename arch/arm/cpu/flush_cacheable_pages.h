/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2024 Ahmad Fatoum, Pengutronix */

/**
 * flush_cacheable_pages - Flush only the cacheable pages in a region
 * @start: Starting virtual address of the range.
 * @size:  Size of range
 *
 * This function walks the page table and flushes the data caches for the
 * specified range only if the memory is marked as normal cacheable in the
 * page tables. If a non-cacheable or non-normal page is encountered,
 * it's skipped.
 */
/**
 * flush_cacheable_pages - Flush only the cacheable pages in a region
 * @start: Starting virtual address of the range.
 * @size:  Size of range
 *
 * This function walks the page table and flushes the data caches for the
 * specified range only if the memory is marked as normal cacheable in the
 * page tables. If a non-cacheable or non-normal page is encountered,
 * it's skipped.
 */
static void flush_cacheable_pages(void *start, size_t size)
{
	mmu_addr_t flush_start = ~0UL, flush_end = ~0UL;
	mmu_addr_t region_start, region_end;
	size_t block_size;
	mmu_addr_t *ttb;

	region_start = PAGE_ALIGN_DOWN((ulong)start);
	region_end = PAGE_ALIGN(region_start + size) - 1;

	ttb = get_ttb();

	/*
	 * TODO: This loop could be made more optimal by inlining the page walk,
	 * so we need not restart address translation from the top every time.
	 *
	 * The hope is that with the page tables being cached and the
	 * windows being remapped being small, the overhead compared to
	 * actually flushing the ranges isn't too significant.
	 */
	for (mmu_addr_t addr = region_start; addr < region_end; addr += block_size) {
		unsigned level;
		mmu_addr_t *pte = find_pte(ttb, addr, &level);

		block_size = granule_size(level);

		if (!pte || !pte_is_cacheable(*pte, level))
			continue;

		if (flush_end == addr) {
			/*
			 * While it's safe to flush the whole block_size,
			 * it's unnecessary time waste to go beyond region_end.
			 */
			flush_end = min(flush_end + block_size, region_end);
			continue;
		}

		/*
		 * We don't have a previous contiguous flush area to append to.
		 * If we recorded any area before, let's flush it now
		 */
		if (flush_start != ~0UL)
			dma_flush_range_end(flush_start, flush_end);

		/* and start the new contiguous flush area with this page */
		flush_start = addr;
		flush_end = min(flush_start + block_size, region_end);
	}

	/* The previous loop won't flush the last cached range, so do it here */
	if (flush_start != ~0UL)
		dma_flush_range_end(flush_start, flush_end);
}
