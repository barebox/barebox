#ifndef __MEMTEST_H
#define __MEMTEST_H

#include <linux/ioport.h>

struct mem_test_resource {
	struct resource *r;
	struct list_head list;
};

int mem_test_request_regions(struct list_head *list);
void mem_test_release_regions(struct list_head *list);
struct mem_test_resource *mem_test_biggest_region(struct list_head *list);

int mem_test_bus_integrity(resource_size_t _start, resource_size_t _end);
int mem_test_moving_inversions(resource_size_t _start, resource_size_t _end);

#endif /* __MEMTEST_H */
