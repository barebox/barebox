/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MEM_MALLOC_H
#define __MEM_MALLOC_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/ioport.h>

void mem_malloc_init(void *start, void *end);
ulong mem_malloc_start(void);
ulong mem_malloc_end(void);

static inline ulong mem_malloc_size(void)
{
	return mem_malloc_end() - mem_malloc_start() + 1;
}

struct memory_bank {
	struct list_head list;
	unsigned long start;
	unsigned long size;
	struct resource *res;
};

extern struct list_head memory_banks;

int barebox_add_memory_bank(const char *name, resource_size_t start,
				    resource_size_t size);

#define for_each_memory_bank(mem)	list_for_each_entry(mem, &memory_banks, list)
#define for_each_reserved_region(mem, rsv) \
	list_for_each_entry(rsv, &(mem)->res->children, sibling) \
	if (((rsv)->flags & IORESOURCE_BUSY))

struct resource *__request_sdram_region(const char *name, unsigned flags,
					resource_size_t start, resource_size_t size);

static inline struct resource *request_sdram_region(const char *name,
						    resource_size_t start,
						    resource_size_t size)
{
	/* IORESOURCE_MEM is implicit for all SDRAM regions */
	return __request_sdram_region(name, 0, start, size);
}

struct resource *reserve_sdram_region(const char *name, resource_size_t start,
				      resource_size_t size);

int release_sdram_region(struct resource *res);

void memory_bank_find_space(struct memory_bank *bank, resource_size_t *retstart,
			    resource_size_t *retend);
int memory_bank_first_find_space(resource_size_t *retstart,
				 resource_size_t *retend);

static inline u64 memory_sdram_size(unsigned int cols,
				    unsigned int rows,
				    unsigned int banks,
				    unsigned int width)
{
	return (u64)banks * width << (rows + cols);
}

void register_barebox_area(resource_size_t start, resource_size_t size);

struct resource *request_barebox_region(const char *name,
					resource_size_t start,
					resource_size_t size);

#endif
