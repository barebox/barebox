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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <command.h>
#include <getopt.h>
#include <asm/mmu.h>
#include <memory.h>
#include <malloc.h>
#include <common.h>
#include <errno.h>

#include <memtest.h>

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

static int request_memtest_regions(struct list_head *list)
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
			end = PAGE_ALIGN_DOWN(bank->res->end) - 1;
			size = end - start + 1;

			ret = alloc_memtest_region(list, start, size);
			if (ret < 0)
				return ret;

			continue;
		}

		/*
		 * We assume that the regions are sorted in this list
		 * So the first element has start boundary on bank->res->start
		 * and the last element hast end boundary on bank->res->end
		 */
		list_for_each_entry(r, &bank->res->children, sibling) {
			/*
			 * Do on head element for bank boundary
			 */
			if (r->sibling.prev == &bank->res->children) {
				/*
				 * remember last used element
				 */
				start = PAGE_ALIGN(bank->res->start);
				end = PAGE_ALIGN_DOWN(r->start) - 1;
				size = end - start + 1;
				r_prev = r;
				if (start >= end)
					continue;

				ret = alloc_memtest_region(list, start, size);
				if (ret < 0)
					return ret;
				continue;
			}
			/*
			 * Between used regions
			 */
			start = PAGE_ALIGN(r_prev->end);
			end = PAGE_ALIGN_DOWN(r->start) - 1;
			size = end - start + 1;
			r_prev = r;
			if (start >= end)
				continue;

			ret = alloc_memtest_region(list, start, size);
			if (ret < 0)
				return ret;

			if (list_is_last(&r->sibling, &bank->res->children)) {
				/*
				 * Do on head element for bank boundary
				 */
				start = PAGE_ALIGN(r->end);
				end = PAGE_ALIGN_DOWN(bank->res->end) - 1;
				size = end - start + 1;
				if (start >= end)
					continue;

				ret = alloc_memtest_region(list, start, size);
				if (ret < 0)
					return ret;
			}
		}
	}

	return 0;
}

static int __do_memtest(struct list_head *memtest_regions,
		int bus_only, uint32_t cache_flag)
{
	struct mem_test_resource *r;
	int ret;

	list_for_each_entry(r, memtest_regions, list) {
		printf("Testing memory space: "
				"0x%08x -> 0x%08x:\n",
				r->r->start,  r->r->end);
		remap_range((void *)r->r->start, r->r->end -
				r->r->start + 1, cache_flag);

		ret = mem_test(r->r->start, r->r->end, bus_only);
		if (ret < 0)
			return ret;
		printf("done.\n\n");
	}

	return 0;
}

static int do_memtest(int argc, char *argv[])
{
	int bus_only = 0, ret, opt;
	uint32_t i, max_i = 1, pte_flags_cached, pte_flags_uncached;
	struct mem_test_resource *r, *r_tmp;
	struct list_head memtest_used_regions;

	while ((opt = getopt(argc, argv, "i:b")) > 0) {
		switch (opt) {
		case 'i':
			max_i = simple_strtoul(optarg, NULL, 0);
			break;
		case 'b':
			bus_only = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind > argc)
		return COMMAND_ERROR_USAGE;

	/*
	 * Get pte flags for enable and disable cache support on page.
	 */
	pte_flags_cached = mmu_get_pte_cached_flags();
	pte_flags_uncached = mmu_get_pte_uncached_flags();

	INIT_LIST_HEAD(&memtest_used_regions);

	ret = request_memtest_regions(&memtest_used_regions);
	if (ret < 0)
		goto out;

	for (i = 1; (i <= max_i) || !max_i; i++) {
		if (max_i)
			printf("Start iteration %u of %u.\n", i, max_i);
		/*
		 * First try a memtest with caching enabled.
		 */
		if (IS_ENABLED(CONFIG_MMU)) {
			printf("Do memtest with caching enabled.\n");
			ret = __do_memtest(&memtest_used_regions,
					bus_only, pte_flags_cached);
			if (ret < 0)
				goto out;
		}
		/*
		 * Second try a memtest with caching disabled.
		 */
		printf("Do memtest with caching disabled.\n");
		ret = __do_memtest(&memtest_used_regions,
				bus_only, pte_flags_uncached);
		if (ret < 0)
			goto out;
	}

out:
	list_for_each_entry_safe(r, r_tmp, &memtest_used_regions, list) {
		/*
		 * Ensure to leave with a cached on non used sdram regions.
		 */
		remap_range((void *)r->r->start, r->r->end -
				r->r->start + 1, pte_flags_cached);
		release_sdram_region(r->r);
		free(r);
	}

	if (ret < 0) {
		/*
		 * Set cursor to newline, because mem_test failed at
		 * drawing of progressbar.
		 */
		if (ret == -EINTR)
			printf("\n");

		printf("Memtest failed.\n");
		return 1;
	} else {
		printf("Memtest successful.\n");
		return 0;
	}
}

static const __maybe_unused char cmd_memtest_help[] =
"Usage: memtest [OPTION]...\n"
"memtest related commands\n"
"	-i	<iterations>	iterations [default=1, endless=0].\n"
"	-b			perform only a test on buslines.";

BAREBOX_CMD_START(memtest)
	.cmd		= do_memtest,
	.usage		= "Memory Test",
	BAREBOX_CMD_HELP(cmd_memtest_help)
BAREBOX_CMD_END
