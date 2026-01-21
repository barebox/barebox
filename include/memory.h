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
	struct resource *res;
};

extern struct list_head memory_banks;

int barebox_add_memory_bank(const char *name, resource_size_t start,
				    resource_size_t size);

#define for_each_memory_bank(mem)	list_for_each_entry(mem, &memory_banks, list)
#define for_each_memory_bank_reverse(mem)	\
	list_for_each_entry_reverse(mem, &memory_banks, list)
#define for_each_reserved_region(mem, rsv) \
	list_for_each_entry(rsv, &(mem)->res->children, sibling) \
		if (!is_reserved_resource(rsv)) {} else

#define for_each_memory_bank_region(bank, region) \
	for_each_resource_region((bank)->res, region)

#define for_each_memory_bank_region_reverse(bank, region) \
	for_each_resource_region_reverse((bank)->res, region)

struct resource *__request_sdram_region(const char *name,
					resource_size_t start, resource_size_t size,
					bool verbose);

static inline struct resource *sdram_region_with_attrs(struct resource *res,
						       enum resource_memtype memtype,
						       unsigned memattrs)
{
	if (IS_ENABLED(CONFIG_MEMORY_ATTRIBUTES) && res) {
		res->type = memtype;
		res->attrs = memattrs;
		res->flags |= IORESOURCE_TYPE_VALID;
	}

	return res;
}

static inline struct resource *request_sdram_region(const char *name,
						    resource_size_t start,
						    resource_size_t size,
						    enum resource_memtype memtype,
						    unsigned memattrs)
{
	struct resource *res;

	/* IORESOURCE_MEM is implicit for all SDRAM regions */
	res = __request_sdram_region(name, start, size, true);

	return sdram_region_with_attrs(res, memtype, memattrs);
}

static inline struct resource *request_sdram_region_silent(const char *name,
							   resource_size_t start,
							   resource_size_t size,
							   enum resource_memtype memtype,
							   unsigned memattrs)
{
	struct resource *res;

	/* IORESOURCE_MEM is implicit for all SDRAM regions */
	res = __request_sdram_region(name, start, size, false);

	return sdram_region_with_attrs(res, memtype, memattrs);
}

struct resource *reserve_sdram_region(const char *name, resource_size_t start,
				      resource_size_t size);

/* It's always fine to call release_region directly as well */
static inline int release_sdram_region(struct resource *res)
{
	return release_region(res);
}

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

#if IN_PROPER
bool inside_barebox_area(resource_size_t start, resource_size_t end);
struct resource *request_barebox_region(const char *name,
					resource_size_t start,
					resource_size_t size,
					unsigned memattrs);
#else
static inline struct resource *request_barebox_region(const char *name,
					resource_size_t start,
					resource_size_t size,
					unsigned memattrs)
{

		return NULL;
}

static inline bool inside_barebox_area(resource_size_t start, resource_size_t end)
{
	return false;
}
#endif

#endif
