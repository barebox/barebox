// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#define pr_fmt(fmt) "memory: " fmt

#include <common.h>
#include <memory.h>
#include <of.h>
#include <init.h>
#include <linux/ioport.h>
#include <linux/err.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <malloc.h>
#include <of.h>
#include <mmu.h>

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static unsigned long malloc_start;
static unsigned long malloc_end;
static unsigned long malloc_brk;

unsigned long mem_malloc_start(void)
{
	return malloc_start;
}

unsigned long mem_malloc_end(void)
{
	return malloc_end;
}

#ifdef CONFIG_MALLOC_TLSF
#include <tlsf.h>
tlsf_t tlsf_mem_pool;
#endif

int mem_malloc_initialized;

int mem_malloc_is_initialized(void)
{
	return mem_malloc_initialized;
}

void mem_malloc_init(void *start, void *end)
{
	malloc_start = (unsigned long)start;
	malloc_end = (unsigned long)end;
	malloc_brk = malloc_start;
#ifdef CONFIG_MALLOC_TLSF
	tlsf_mem_pool = tlsf_create_with_pool(start, end - start + 1);
#endif
	mem_malloc_initialized = 1;
}

static struct resource *barebox_res;
static resource_size_t barebox_start;
static resource_size_t barebox_size;

void register_barebox_area(resource_size_t start,
			   resource_size_t size)
{
	barebox_start = start,
	barebox_size = size;
}

static int mem_register_barebox(void)
{
	if (barebox_start && barebox_size)
		barebox_res = request_sdram_region("barebox", barebox_start,
						   barebox_size);
	return 0;
}
postmem_initcall(mem_register_barebox);

struct resource *request_barebox_region(const char *name,
					resource_size_t start,
					resource_size_t size)
{
	resource_size_t end = start + size - 1;

	if (barebox_res && barebox_res->start <= start &&
	    end <= barebox_res->end)
		return __request_region(barebox_res, start, end,
					name, IORESOURCE_MEM);

	return request_sdram_region(name, start, size);
}

static int mem_malloc_resource(void)
{
#if !defined __SANDBOX__
	/*
	 * Normally it's a bug when one of these fails,
	 * but we have some setups where some of these
	 * regions are outside of sdram in which case
	 * the following fails.
	 */
	request_sdram_region("malloc space",
			malloc_start,
			malloc_end - malloc_start + 1);
	request_barebox_region("barebox code",
			(unsigned long)&_stext,
			(unsigned long)&_etext -
			(unsigned long)&_stext);
	request_barebox_region("barebox data",
			(unsigned long)&_sdata,
			(unsigned long)&_edata -
			(unsigned long)&_sdata);
	request_barebox_region("barebox bss",
			(unsigned long)&__bss_start,
			(unsigned long)&__bss_stop -
			(unsigned long)&__bss_start);
#endif
#ifdef STACK_BASE
	request_sdram_region("stack", STACK_BASE, STACK_SIZE);
#endif

	return 0;
}
coredevice_initcall(mem_malloc_resource);

static void *sbrk_no_zero(ptrdiff_t increment)
{
	unsigned long old = malloc_brk;
	unsigned long new = old + increment;

	if ((new < malloc_start) || (new > malloc_end))
		return NULL;

	malloc_brk = new;

	return (void *)old;
}

void *sbrk(ptrdiff_t increment)
{
	void *old = sbrk_no_zero(increment);

	/* Only clear increment, if valid address was returned */
	if (old != NULL)
		memset(old, 0, increment);

	return old;
}

LIST_HEAD(memory_banks);

static int barebox_grow_memory_bank(struct memory_bank *bank, const char *name,
				    const struct resource *newres)
{
	struct resource *res;
	resource_size_t bank_end = bank->res->end;

	if (newres->start < bank->start) {
		res = request_iomem_region(name, newres->start, bank->start - 1);
		if (IS_ERR(res))
			return PTR_ERR(res);
		__merge_regions(name, bank->res, res);
	}

	if (bank_end < newres->end) {
		res = request_iomem_region(name, bank_end + 1, newres->end);
		if (IS_ERR(res))
			return PTR_ERR(res);
		__merge_regions(name, bank->res, res);
	}

	bank->start = newres->start;
	bank->size = resource_size(bank->res);

	return 0;
}

int barebox_add_memory_bank(const char *name, resource_size_t start,
				    resource_size_t size)
{
	struct memory_bank *bank;
	struct resource *res;
	struct resource newres = {
		.start = start,
		.end = start + size - 1,
		.flags = IORESOURCE_MEM,
	};

	for_each_memory_bank(bank) {
		if (resource_contains(bank->res, &newres))
			return 0;
		if (resource_contains(&newres, bank->res))
			return barebox_grow_memory_bank(bank, name, &newres);
	}

	res = request_iomem_region(name, start, start + size - 1);
	if (IS_ERR(res))
		return PTR_ERR(res);

	res->flags = IORESOURCE_MEM;

	bank = xzalloc(sizeof(*bank));

	bank->res = res;
	bank->start = start;
	bank->size = size;

	list_add_tail(&bank->list, &memory_banks);

	return 0;
}

static int add_mem_devices(void)
{
	struct memory_bank *bank;

	for_each_memory_bank(bank) {
		add_mem_device(bank->res->name, bank->start, bank->size,
			       IORESOURCE_MEM_WRITEABLE);
	}

	return 0;
}
postmem_initcall(add_mem_devices);

/*
 * Request a region from the registered sdram
 */
struct resource *__request_sdram_region(const char *name, unsigned flags,
					resource_size_t start, resource_size_t size)
{
	struct memory_bank *bank;

	flags |= IORESOURCE_MEM;

	for_each_memory_bank(bank) {
		struct resource *res;

		res = __request_region(bank->res, start, start + size - 1,
				       name, flags);
		if (!IS_ERR(res))
			return res;
	}

	return NULL;
}

/* use for secure firmware to inhibit speculation */
struct resource *reserve_sdram_region(const char *name, resource_size_t start,
				      resource_size_t size)
{
	struct resource *res;

	if (!IS_ALIGNED(start, PAGE_SIZE)) {
		pr_err("%s: %s start is not page aligned\n", __func__, name);
		start = ALIGN_DOWN(start, PAGE_SIZE);
	}

	if (!IS_ALIGNED(size, PAGE_SIZE)) {
		pr_err("%s: %s size is not page aligned\n", __func__, name);
		size = ALIGN(size, PAGE_SIZE);
	}

	res = __request_sdram_region(name, IORESOURCE_BUSY, start, size);
	if (!res)
		return NULL;

	remap_range((void *)start, size, MAP_UNCACHED);

	return res;
}

int release_sdram_region(struct resource *res)
{
	return release_region(res);
}

void memory_bank_find_space(struct memory_bank *bank, resource_size_t *retstart,
			   resource_size_t *retend)
{
	resource_size_t freeptr, size, maxfree = 0;
	struct resource *last, *child;

	if (list_empty(&bank->res->children)) {
		/* No children - return the whole bank */
		*retstart = bank->res->start;
		*retend = bank->res->end;
		return;
	}

	freeptr = bank->res->start;

	list_for_each_entry(child, &bank->res->children, sibling) {
		/* Check gaps between child resources */
		size = child->start - freeptr;
		if (size > maxfree) {
			*retstart = freeptr;
			*retend = child->start - 1;
			maxfree = size;
		}
		freeptr = child->start + resource_size(child);
	}

	last = list_last_entry(&bank->res->children, struct resource, sibling);

	/* Check gap between last child and end of memory bank */
	freeptr = last->start + resource_size(last);
	size = bank->res->start + resource_size(bank->res) - freeptr;

	if (size > maxfree) {
		*retstart = freeptr;
		*retend = bank->res->end;
	}
}

int memory_bank_first_find_space(resource_size_t *retstart,
				 resource_size_t *retend)
{
	struct memory_bank *bank;

	for_each_memory_bank(bank) {
		memory_bank_find_space(bank, retstart, retend);
		return 0;
	}

	return -ENOENT;
}

#ifdef CONFIG_OFTREE

static int of_memory_fixup(struct device_node *root, void *unused)
{
	struct memory_bank *bank;
	int err;
	int addr_cell_len, size_cell_len;
	struct device_node *memnode, *tmp, *np;
	char *memnode_name;

	/*
	 * Since kernel 4.16 the memory node got a @<reg> suffix. To support
	 * the old and the new style delete any found memory node and add it
	 * again to be sure that the memory node exists only once. It shouldn't
	 * bother older kernels if the memory node has this suffix so adding it
	 * following the new style.
	 */

	for_each_child_of_node_safe(root, tmp, np) {
		const char *device_type;

		err = of_property_read_string(np, "device_type", &device_type);
		if (err || of_node_cmp("memory", device_type))
			continue;

		/* delete every found memory node */
		of_delete_node(np);
	}

	addr_cell_len = of_n_addr_cells(root);
	size_cell_len = of_n_size_cells(root);

	for_each_memory_bank(bank) {
		u8 tmp[16]; /* Up to 64-bit address + 64-bit size */
		int len = 0;

		/* Create a /memory node for each bank */
		memnode_name = basprintf("/memory@%lx", bank->start);
		if (!memnode_name) {
			err = -ENOMEM;
			goto err_out;
		}

		memnode = of_create_node(root, memnode_name);
		if (!memnode) {
			err = -ENOMEM;
			goto err_free;
		}

		err = of_property_write_string(memnode, "device_type",
					       "memory");
		if (err)
			goto err_free;

		of_write_number(tmp, bank->start, addr_cell_len);
		len += addr_cell_len * 4;
		of_write_number(tmp + len, bank->size, size_cell_len);
		len += size_cell_len * 4;

		err = of_set_property(memnode, "reg", tmp, len, 1);
		if (err) {
			pr_err("could not set reg %s.\n", strerror(-err));
			goto err_free;
		}

		free(memnode_name);
	}

	return 0;

err_free:
	free(memnode_name);
err_out:
	return err;
}

static int of_register_memory_fixup(void)
{
	return of_register_fixup(of_memory_fixup, NULL);
}
late_initcall(of_register_memory_fixup);
#endif
