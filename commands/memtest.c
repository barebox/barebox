/*
 * memtest - Perform a memory test
 *
 * (C) Copyright 2013
 * Alexander Aring <aar@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <command.h>
#include <getopt.h>
#include <memory.h>
#include <malloc.h>
#include <common.h>
#include <errno.h>
#include <memtest.h>
#include <mmu.h>

static int do_test_one_area(struct mem_test_resource *r, int bus_only,
		unsigned cache_flag)
{
	int ret;

	printf("Testing memory space: %pa -> %pa:\n",
	       &r->r->start,  &r->r->end);

	remap_range((void *)r->r->start, resource_size(r->r), cache_flag);

	ret = mem_test_bus_integrity(r->r->start, r->r->end);
	if (ret < 0)
		return ret;

	if (bus_only)
		return 0;

	ret = mem_test_moving_inversions(r->r->start, r->r->end);
	if (ret < 0)
		return ret;
	printf("done.\n\n");

	return 0;
}

static int do_memtest_thorough(struct list_head *memtest_regions,
		int bus_only, unsigned cache_flag)
{
	struct mem_test_resource *r;
	int ret;

	list_for_each_entry(r, memtest_regions, list) {
		ret = do_test_one_area(r, bus_only, cache_flag);
		if (ret)
			return ret;
	}

	return 0;
}

static int do_memtest_biggest(struct list_head *memtest_regions,
		int bus_only, unsigned cache_flag)
{
	struct mem_test_resource *r;

	r = mem_test_biggest_region(memtest_regions);
	if (!r)
		return -EINVAL;

	return do_test_one_area(r, bus_only, cache_flag);
}

static int do_memtest(int argc, char *argv[])
{
	int bus_only = 0, ret, opt;
	uint32_t i, max_i = 1;
	struct list_head memtest_used_regions;
	int (*memtest)(struct list_head *, int, unsigned);
	int cached = 0, uncached = 0;

	memtest = do_memtest_biggest;

	while ((opt = getopt(argc, argv, "i:btcu")) > 0) {
		switch (opt) {
		case 'i':
			max_i = simple_strtoul(optarg, NULL, 0);
			break;
		case 'b':
			bus_only = 1;
			break;
		case 't':
			memtest = do_memtest_thorough;
			break;
		case 'c':
			cached = 1;
			break;
		case 'u':
			uncached = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!arch_can_remap() && (cached || uncached)) {
		printf("Cannot map cached or uncached\n");
		return -EINVAL;
	}

	if (optind > argc)
		return COMMAND_ERROR_USAGE;

	INIT_LIST_HEAD(&memtest_used_regions);

	ret = mem_test_request_regions(&memtest_used_regions);
	if (ret < 0)
		goto out;

	for (i = 1; (i <= max_i) || !max_i; i++) {
		printf("Start iteration %u", i);
		if (max_i)
			printf(" of %u.\n", max_i);
		else
			putchar('\n');

		if (cached) {
			printf("Do memtest with caching enabled.\n");
			ret = memtest(&memtest_used_regions,
					bus_only, MAP_CACHED);
			if (ret < 0)
				goto out;
		}

		if (uncached) {
			printf("Do memtest with caching disabled.\n");
			ret = memtest(&memtest_used_regions,
					bus_only, MAP_UNCACHED);
			if (ret < 0)
				goto out;
		}

		if (!cached && !uncached) {
			ret = memtest(&memtest_used_regions,
					bus_only, MAP_DEFAULT);
			if (ret < 0)
				goto out;
		}
	}

out:
	mem_test_release_regions(&memtest_used_regions);

	if (ret < 0) {
		/*
		 * Set cursor to newline, because mem_test failed at
		 * drawing of progressbar.
		 */
		if (ret == -EINTR)
			printf("\n");

		printf("Memtest failed. Error: %d\n", ret);
		return 1;
	}

	printf("Memtest successful.\n");
	return 0;
}


BAREBOX_CMD_HELP_START(memtest)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-i ITERATIONS", "perform number of iterations (default 1, 0 is endless)")
BAREBOX_CMD_HELP_OPT("-b", "perform only a test on bus lines")
BAREBOX_CMD_HELP_OPT("-c", "cached. Test using cached memory")
BAREBOX_CMD_HELP_OPT("-u", "uncached. Test using uncached memory")
BAREBOX_CMD_HELP_OPT("-t", "thorough. test all free areas. If unset, only test biggest free area")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(memtest)
	.cmd		= do_memtest,
	BAREBOX_CMD_DESC("extensive memory test")
	BAREBOX_CMD_OPTS("[-ibcut]")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_memtest_help)
BAREBOX_CMD_END
