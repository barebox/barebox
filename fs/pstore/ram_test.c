// SPDX-License-Identifier: GPL-2.0-only
/*
 * Test how much of the ramoops area survives a reset
 *
 * Instead of serving pstore, the area is filled with the same pseudo-random
 * pattern on every start, after comparing what the previous start left in
 * it against that pattern.
 */

#define pr_fmt(fmt) "ramoops: " fmt

#include <dma.h>
#include <memory.h>
#include <of.h>
#include <stdlib.h>
#include <string.h>
#include <asm/unaligned.h>
#include <linux/array_size.h>
#include <linux/bitops.h>
#include <linux/ioport.h>
#include <linux/math64.h>
#include <linux/minmax.h>
#include <linux/printk.h>

#include "ram_internal.h"

/* Seeds the pattern, so it's the same across resets */
#define RAMOOPS_TEST_SEED	"OOPSTEST"

/* Report lost bytes less than a cache line apart as one range */
#define RAMOOPS_TEST_GAP	64
#define RAMOOPS_TEST_RANGES	16

static void ramoops_test_compare(const u8 *area, phys_addr_t start,
				 size_t size)
{
	struct {
		size_t start, end;
	} lost[RAMOOPS_TEST_RANGES];
	size_t nbad = 0, nlost = 0, to0 = 0, to1 = 0, last = 0;
	u64 state = get_unaligned_be64(RAMOOPS_TEST_SEED);
	unsigned int permyriad;
	size_t off, i;
	u8 expect[256];

	for (off = 0; off < size; off += sizeof(expect)) {
		size_t len = min(size - off, sizeof(expect));
		const u8 *actual = area + off;

		randbuf_r(&state, expect, len);
		if (!memcmp(actual, expect, len))
			continue;

		for (i = 0; i < len; i++) {
			size_t pos = off + i;

			if (actual[i] == expect[i])
				continue;

			nbad++;
			to0 += hweight8(expect[i] & ~actual[i]);
			to1 += hweight8(~expect[i] & actual[i]);

			if (nlost && pos < last + RAMOOPS_TEST_GAP) {
				if (nlost <= ARRAY_SIZE(lost))
					lost[nlost - 1].end = pos + 1;
			} else {
				if (nlost < ARRAY_SIZE(lost)) {
					lost[nlost].start = pos;
					lost[nlost].end = pos + 1;
				}
				nlost++;
			}
			last = pos + 1;
		}
	}

	permyriad = div64_u64((u64)(size - nbad) * 10000, size);

	pr_info("retention test: %zu of %zu bytes intact (%u.%02u%%)\n",
		size - nbad, size, permyriad / 100, permyriad % 100);

	if (!nbad)
		return;

	pr_info("%zu bits flipped from 1 to 0, %zu from 0 to 1\n", to0, to1);

	for (i = 0; i < min(nlost, ARRAY_SIZE(lost)); i++)
		pr_info("lost 0x%08llx - 0x%08llx (0x%zx bytes)\n",
			(unsigned long long)(start + lost[i].start),
			(unsigned long long)(start + lost[i].end - 1),
			lost[i].end - lost[i].start);

	if (nlost > ARRAY_SIZE(lost))
		pr_info("lost %zu more ranges\n", nlost - ARRAY_SIZE(lost));
}

static int ramoops_test_of_fixup(struct device_node *root, void *data)
{
	struct device_node *node;
	int ret;

	ret = of_fixup_reserved_memory(root, data);
	if (ret)
		return ret;

	/* Keep a ramoops driver in the kernel from writing to the area */
	node = of_find_node_by_path_from(root, "/reserved-memory/ramoops");
	if (node)
		of_delete_property_by_name(node, "compatible");

	return 0;
}

int ramoops_retention_test(phys_addr_t start, size_t size)
{
	static struct resource fixup_res = { .name = "ramoops" };
	u64 state = get_unaligned_be64(RAMOOPS_TEST_SEED);
	struct resource *res;
	void *area;

	res = request_barebox_region("ramoops:test", start, size,
				     MEMATTRS_RW | MEMATTR_SP);
	if (!res) {
		pr_err("failed to request 0x%zx@0x%llx\n", size,
		       (unsigned long long)start);
		return -EBUSY;
	}

	res->type = MEMTYPE_PERSISTENT;
	res->runtime = true;
	area = (void *)start;

	ramoops_test_compare(area, start, size);

	randbuf_r(&state, area, size);
	/* A reset needn't write back the caches, so push it all out to RAM */
	arch_sync_dma_for_device(area, size, DMA_TO_DEVICE);

	fixup_res.start = start;
	fixup_res.end = start + size - 1;
	of_register_fixup(ramoops_test_of_fixup, &fixup_res);

	pr_info("wrote retention test pattern to 0x%zx@0x%llx, pstore disabled\n",
		size, (unsigned long long)start);

	return 0;
}
