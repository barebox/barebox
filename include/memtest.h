/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MEMTEST_H
#define __MEMTEST_H

#include <linux/ioport.h>
#include <linux/bitops.h>

struct mem_test_resource {
	struct resource *r;
	struct list_head list;
};

int mem_test_request_regions(struct list_head *list);
void mem_test_release_regions(struct list_head *list);
struct mem_test_resource *mem_test_biggest_region(struct list_head *list);

#define MEMTEST_VERBOSE		BIT(0)

int mem_test_bus_integrity(resource_size_t _start, resource_size_t _end, unsigned flags);
int mem_test_moving_inversions(resource_size_t _start, resource_size_t _end, unsigned flags);

#endif /* __MEMTEST_H */
