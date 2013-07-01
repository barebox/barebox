#ifndef __MEMTEST_H
#define __MEMTEST_H

#include <linux/ioport.h>

struct mem_test_resource {
	struct resource *r;
	struct list_head list;
};

int mem_test(resource_size_t _start,
		resource_size_t _end, int bus_only);

#endif /* __MEMTEST_H */
