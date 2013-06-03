/*
 * memory_test.c
 *
 * Copyright (c) 2013 Alexander Aring <aar@pengutronix.de>, Pengutronix
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
#include <sizes.h>
#include <memtest.h>

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

/*
 * Perform a memory test. The complete test
 * loops until interrupted by ctrl-c.
 *
 * Prameters:
 * start: start address for memory test.
 * end: end address of memory test.
 * bus_only: skip integrity check and do only a address/data bus
 *	     testing.
 *
 * Return value can be -EINVAL for invalid parameter or -EINTR
 * if memory test was interrupted.
 */
int mem_test(resource_size_t _start,
	       resource_size_t _end, int bus_only)
{
	volatile resource_size_t *start, *dummy, val, readback, offset,
		offset2, pattern, temp, anti_pattern, num_words;
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
	for (i = 0; i < ARRAY_SIZE(bitpattern)/
			sizeof(resource_size_t); i++) {
		val = bitpattern[i];

		for (; val != 0; val <<= 1) {
			*start = val;
			/* clear the test data off of the bus */
			*dummy = ~val;
			readback = *start;
			if (readback != val) {
				printf("FAILURE (data line): "
					"expected 0x%08x, actual 0x%08x at address 0x%08x.\n",
					val, readback, (resource_size_t)start);
				return -EIO;
			}

			*start = ~val;
			*dummy = val;
			readback = *start;
			if (readback != ~val) {
				printf("FAILURE (data line): "
					"Is 0x%08x, should be 0x%08x at address 0x%08x.\n",
					readback,
					~val, (resource_size_t)start);
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

	printf("Check for address bits stuck high.\n");

	/*
	 * Check for address bits stuck high.
	 */
	for (offset = 1; offset <= num_words; offset <<= 1) {
		temp = start[offset];
		if (temp != pattern) {
			printf("FAILURE: Address bit "
					"stuck high @ 0x%08x:"
					" expected 0x%08x, actual 0x%08x.\n",
					(resource_size_t)&start[offset],
					pattern, temp);
			return -EIO;
		}
	}

	printf("Check for address bits stuck "
			"low or shorted.\n");

	/*
	 * Check for address bits stuck low or shorted.
	 */
	for (offset2 = 1; offset2 <= num_words; offset2 <<= 1) {
		start[offset2] = anti_pattern;

		for (offset = 1; offset <= num_words; offset <<= 1) {
			temp = start[offset];

			if ((temp != pattern) &&
					(offset != offset2)) {
				printf("FAILURE: Address bit stuck"
						" low or shorted @"
						" 0x%08x: expected 0x%08x, actual 0x%08x.\n",
						(resource_size_t)&start[offset],
						pattern, temp);
				return -EIO;
			}
		}
		start[offset2] = pattern;
	}

	/*
	 * We tested only the bus if != 0
	 * leaving here
	 */
	if (bus_only)
		return 0;

	printf("Starting integrity check of physicaly ram.\n"
			"Filling ram with patterns...\n");

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

	/*
	 * Fill memory with a known pattern.
	 */
	init_progression_bar(num_words);
	for (offset = 0; offset < num_words; offset++) {
		/*
		 * Every 4K we update the progressbar.
		 */
		if (!(offset & (SZ_4K - 1))) {
			if (ctrlc())
				return -EINTR;
			show_progress(offset);
		}

		start[offset] = offset + 1;
	}
	show_progress(offset);

	printf("\nCompare written patterns...\n");
	/*
	 * Check each location and invert it for the second pass.
	 */
	init_progression_bar(num_words - 1);
	for (offset = 0; offset < num_words; offset++) {
		if (!(offset & (SZ_4K - 1))) {
			if (ctrlc())
				return -EINTR;
			show_progress(offset);
		}

		temp = start[offset];
		if (temp != (offset + 1)) {
			printf("\nFAILURE (read/write) @ 0x%08x:"
					" expected 0x%08x, actual 0x%08x.\n",
					(resource_size_t)&start[offset],
					(offset + 1), temp);
			return -EIO;
		}

		anti_pattern = ~(offset + 1);
		start[offset] = anti_pattern;
	}
	show_progress(offset);

	printf("\nFilling ram with inverted pattern and compare it...\n");
	/*
	 * Check each location for the inverted pattern and zero it.
	 */
	init_progression_bar(num_words - 1);
	for (offset = 0; offset < num_words; offset++) {
		if (!(offset & (SZ_4K - 1))) {
			if (ctrlc())
				return -EINTR;
			show_progress(offset);
		}

		anti_pattern = ~(offset + 1);
		temp = start[offset];

		if (temp != anti_pattern) {
			printf("\nFAILURE (read/write): @ 0x%08x:"
					" expected 0x%08x, actual 0x%08x.\n",
					(resource_size_t)&start[offset],
					anti_pattern, temp);
			return -EIO;
		}

		start[offset] = 0;
	}
	show_progress(offset);

	/*
	 * end of progressbar
	 */
	printf("\n");

	return 0;
}
