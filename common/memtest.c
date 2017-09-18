/*
 * memtest.c
 *
 * Copyright (C) 2013 Alexander Aring <aar@pengutronix.de>, Pengutronix
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <progress.h>
#include <common.h>
#include <memory.h>
#include <types.h>
#include <linux/sizes.h>
#include <errno.h>
#include <memtest.h>
#include <malloc.h>
#include <mmu.h>

static int alloc_memtest_region(struct list_head *list,
		resource_size_t start, resource_size_t size)
{
	struct resource *r_new;
	struct mem_test_resource *r;

	r = xzalloc(sizeof(struct mem_test_resource));
	r_new = request_sdram_region("memtest", start, size);
	if (!r_new)
		return -EINVAL;

	r->r = r_new;
	list_add_tail(&r->list, list);

	return 0;
}

int mem_test_request_regions(struct list_head *list)
{
	int ret;
	struct memory_bank *bank;
	struct resource *r, *r_prev = NULL;
	resource_size_t start, end, size;

	for_each_memory_bank(bank) {
		/*
		 * If we don't have any allocated region on bank,
		 * we use the whole bank boundary
		 */
		if (list_empty(&bank->res->children)) {
			start = PAGE_ALIGN(bank->res->start);
			size = PAGE_ALIGN_DOWN(bank->res->end - start + 1);

			if (size) {
				ret = alloc_memtest_region(list, start, size);
				if (ret < 0)
					return ret;
			}

			continue;
		}

		r = list_first_entry(&bank->res->children,
				     struct resource, sibling);
		start = PAGE_ALIGN(bank->res->start);
		end = PAGE_ALIGN_DOWN(r->start);
		r_prev = r;
		if (start != end) {
			size = end - start;
			ret = alloc_memtest_region(list, start, size);
			if (ret < 0)
				return ret;
		}
		/*
		 * We assume that the regions are sorted in this list
		 * So the first element has start boundary on bank->res->start
		 * and the last element hast end boundary on bank->res->end.
		 *
		 * Between used regions. Start from second entry.
		 */
		list_for_each_entry_continue(r, &bank->res->children, sibling) {
			start = PAGE_ALIGN(r_prev->end + 1);
			end = r->start - 1;
			r_prev = r;
			if (start >= end)
				continue;

			size = PAGE_ALIGN_DOWN(end - start + 1);
			if (size == 0)
				continue;
			ret = alloc_memtest_region(list, start, size);
			if (ret < 0)
				return ret;
		}

		/*
		 * Do on head element for bank boundary.
		 */
		r = list_last_entry(&bank->res->children,
				     struct resource, sibling);
		start = PAGE_ALIGN(r->end);
		end = bank->res->end;
		size = PAGE_ALIGN_DOWN(end - start + 1);
		if (size && start < end && start > r->end) {
			ret = alloc_memtest_region(list, start, size);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

void mem_test_release_regions(struct list_head *list)
{
	struct mem_test_resource *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		/*
		 * Ensure to leave with a cached on non used sdram regions.
		 */
		remap_range((void *)r->r->start, r->r->end -
				r->r->start + 1, MAP_DEFAULT);

		release_sdram_region(r->r);
		free(r);
	}
}

struct mem_test_resource *mem_test_biggest_region(struct list_head *list)
{
	struct mem_test_resource *r, *best = NULL;
	resource_size_t size = 0;

	list_for_each_entry(r, list, list) {
		resource_size_t now = resource_size(r->r);
		if (now > size) {
			size = now;
			best = r;
		}
	}

	return best;
}

static void mem_test_report_failure(const char *failure_description,
				    resource_size_t expected_value,
				    resource_size_t actual_value,
				    volatile resource_size_t *address)
{
	printf("FAILURE (%s): "
	       "expected 0x%08x, actual 0x%08x at address 0x%08x.\n",
	       failure_description, expected_value, actual_value,
	       (resource_size_t)address);
}

int mem_test_bus_integrity(resource_size_t _start,
			   resource_size_t _end)
{
	static const resource_size_t bitpattern[] = {
		0x00000001,	/* single bit */
		0x00000003,	/* two adjacent bits */
		0x00000007,	/* three adjacent bits */
		0x0000000F,	/* four adjacent bits */
		0x00000005,	/* two non-adjacent bits */
		0x00000015,	/* three non-adjacent bits */
		0x00000055,	/* four non-adjacent bits */
		0xAAAAAAAA,	/* alternating 1/0 */
	};

	volatile resource_size_t *start, *dummy, num_words, val, readback, offset,
		offset2, pattern, temp, anti_pattern;
	int i;

	_start = ALIGN(_start, sizeof(resource_size_t));
	_end = ALIGN_DOWN(_end, sizeof(resource_size_t)) - 1;

	if (_end <= _start)
		return -EINVAL;

	start = (resource_size_t *)_start;
		/*
	 * Point the dummy to start[1]
	 */
	dummy = start + 1;
	num_words = (_end - _start + 1)/sizeof(resource_size_t);

	printf("Starting data line test.\n");

	/*
	 * Data line test: write a pattern to the first
	 * location, write the 1's complement to a 'parking'
	 * address (changes the state of the data bus so a
	 * floating bus doen't give a false OK), and then
	 * read the value back. Note that we read it back
	 * into a variable because the next time we read it,
	 * it might be right (been there, tough to explain to
	 * the quality guys why it prints a failure when the
	 * "is" and "should be" are obviously the same in the
	 * error message).
	 *
	 * Rather than exhaustively testing, we test some
	 * patterns by shifting '1' bits through a field of
	 * '0's and '0' bits through a field of '1's (i.e.
	 * pattern and ~pattern).
	 */
	for (i = 0; i < ARRAY_SIZE(bitpattern); i++) {
		val = bitpattern[i];

		for (; val != 0; val <<= 1) {
			*start = val;
			/* clear the test data off of the bus */
			*dummy = ~val;
			readback = *start;
			if (readback != val) {
				mem_test_report_failure("data line",
							val, readback, start);
				return -EIO;
			}

			*start = ~val;
			*dummy = val;
			readback = *start;
			if (readback != ~val) {
				mem_test_report_failure("data line",
							~val, readback, start);
				return -EIO;
			}
		}
	}


	/*
	 * Based on code whose Original Author and Copyright
	 * information follows: Copyright (c) 1998 by Michael
	 * Barr. This software is placed into the public
	 * domain and may be used for any purpose. However,
	 * this notice must not be changed or removed and no
	 * warranty is either expressed or implied by its
	 * publication or distribution.
	 */

	/*
	 * Address line test
	 *
	 * Description: Test the address bus wiring in a
	 *              memory region by performing a walking
	 *              1's test on the relevant bits of the
	 *              address and checking for aliasing.
	 *              This test will find single-bit
	 *              address failures such as stuck -high,
	 *              stuck-low, and shorted pins. The base
	 *              address and size of the region are
	 *              selected by the caller.
	 *
	 * Notes:	For best results, the selected base
	 *              address should have enough LSB 0's to
	 *              guarantee single address bit changes.
	 *              For example, to test a 64-Kbyte
	 *              region, select a base address on a
	 *              64-Kbyte boundary. Also, select the
	 *              region size as a power-of-two if at
	 *              all possible.
	 *
	 * ## NOTE ##	Be sure to specify start and end
	 *              addresses such that num_words has
	 *              lots of bits set. For example an
	 *              address range of 01000000 02000000 is
	 *              bad while a range of 01000000
	 *              01ffffff is perfect.
	 */

	pattern = 0xAAAAAAAA;
	anti_pattern = 0x55555555;

	/*
	 * Write the default pattern at each of the
	 * power-of-two offsets.
	 */
	for (offset = 1; offset <= num_words; offset <<= 1)
		start[offset] = pattern;

	/*
	 * Now write anti-pattern at offset 0. If during the previous
	 * step one of the address lines got stuck high this
	 * operation would result in a memory cell at power-of-two
	 * offset being set to anti-pattern which hopefully would be
	 * detected byt the loop that follows.
	 */
	start[0] = anti_pattern;

	printf("Check for address bits stuck high.\n");

	/*
	 * Check for address bits stuck high.
	 */
	for (offset = 1; offset <= num_words; offset <<= 1) {
		temp = start[offset];
		if (temp != pattern) {
			mem_test_report_failure("address bit stuck high",
						pattern, temp, &start[offset]);
			return -EIO;
		}
	}

	/*
	  Restore original value
	 */
	start[0] = pattern;

	printf("Check for address bits stuck "
			"low or shorted.\n");

	/*
	 * Check for address bits stuck low or shorted.
	 */
	for (offset2 = 1; offset2 <= num_words; offset2 <<= 1) {
		start[offset2] = anti_pattern;

		for (offset = 0; offset <= num_words;
		     offset = (offset) ? offset << 1 : 1) {
			temp = start[offset];

			if ((temp != pattern) &&
					(offset != offset2)) {
				mem_test_report_failure(
					"address bit stuck low or shorted",
					pattern, temp, &start[offset]);
				return -EIO;
			}
		}
		start[offset2] = pattern;
	}

	return 0;
}

static int update_progress(resource_size_t offset)
{
	/* Only check every 4k to reduce overhead */
	if (offset & (SZ_4K - 1))
		return 0;

	if (ctrlc())
		return -EINTR;

	show_progress(offset);

	return 0;
}

int mem_test_moving_inversions(resource_size_t _start, resource_size_t _end)
{
	volatile resource_size_t *start, num_words, offset, temp, anti_pattern;
	int ret;

	_start = ALIGN(_start, sizeof(resource_size_t));
	_end = ALIGN_DOWN(_end, sizeof(resource_size_t)) - 1;

	if (_end <= _start)
		return -EINVAL;

	start = (resource_size_t *)_start;
	num_words = (_end - _start + 1)/sizeof(resource_size_t);

	printf("Starting moving inversions test of RAM:\n"
	       "Fill with address, compare, fill with inverted address, compare again\n");

	/*
	 * Description: Test the integrity of a physical
	 *		memory device by performing an
	 *		increment/decrement test over the
	 *		entire region. In the process every
	 *		storage bit in the device is tested
	 *		as a zero and a one. The base address
	 *		and the size of the region are
	 *		selected by the caller.
	 */

	init_progression_bar(3 * num_words);

	/* Fill memory with a known pattern */
	for (offset = 0; offset < num_words; offset++) {
		ret = update_progress(offset);
		if (ret)
			return ret;
		start[offset] = offset + 1;
	}

	/* Check each location and invert it for the second pass */
	for (offset = 0; offset < num_words; offset++) {
		ret = update_progress(num_words + offset);
		if (ret)
			return ret;

		temp = start[offset];
		if (temp != (offset + 1)) {
			printf("\n");
			mem_test_report_failure("read/write",
						(offset + 1),
						temp, &start[offset]);
			return -EIO;
		}

		anti_pattern = ~(offset + 1);
		start[offset] = anti_pattern;
	}

	/* Check each location for the inverted pattern and zero it */
	for (offset = 0; offset < num_words; offset++) {
		ret = update_progress(2 * num_words + offset);
		if (ret)
			return ret;

		anti_pattern = ~(offset + 1);
		temp = start[offset];

		if (temp != anti_pattern) {
			printf("\n");
			mem_test_report_failure("read/write",
						anti_pattern,
						temp, &start[offset]);
			return -EIO;
		}

		start[offset] = 0;
	}
	show_progress(3 * num_words);

	/* end of progressbar */
	printf("\n");

	return 0;
}
