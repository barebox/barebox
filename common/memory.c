/*
 * memory.c
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <memory.h>
#include <of.h>
#include <init.h>
#include <libfdt.h>

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

void mem_malloc_init(void *start, void *end)
{
	malloc_start = (unsigned long)start;
	malloc_end = (unsigned long)end;
	malloc_brk = malloc_start;
}

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

void barebox_add_memory_bank(const char *name, resource_size_t start,
				    resource_size_t size)
{
	struct memory_bank *bank = xzalloc(sizeof(*bank));
	struct device_d *dev;

	dev = add_mem_device(name, start, size, IORESOURCE_MEM_WRITEABLE);

	bank->dev = dev;
	bank->start = start;
	bank->size = size;

	list_add_tail(&bank->list, &memory_banks);
}

#ifdef CONFIG_OFTREE

/*
 * Get cells len in bytes
 *     if #NNNN-cells property is 2 then len is 8
 *     otherwise len is 4
 */
static int get_cells_len(struct fdt_header *fdt, char *nr_cells_name)
{
	const u32 *cell;

	cell = fdt_getprop(fdt, 0, nr_cells_name, NULL);
	if (cell && *cell == 2)
		return 8;

	return 4;
}

/*
 * Write a 4 or 8 byte big endian cell
 */
static void write_cell(u8 *addr, u64 val, int size)
{
	int shift = (size - 1) * 8;

	while (size-- > 0) {
		*addr++ = (val >> shift) & 0xff;
		shift -= 8;
	}
}

static int of_memory_fixup(struct fdt_header *fdt)
{
	struct memory_bank *bank;
	int err, nodeoffset;
	int addr_cell_len, size_cell_len, len = 0;
	u8 tmp[16 * 16]; /* Up to 64-bit address + 64-bit size */

	/* update, or add and update /memory node */
	nodeoffset = fdt_get_path_or_create(fdt, "/memory");
	if (nodeoffset < 0)
		return nodeoffset;

	err = fdt_setprop(fdt, nodeoffset, "device_type", "memory",
			sizeof("memory"));
	if (err < 0) {
		printf("WARNING: could not set %s %s.\n", "device_type",
				fdt_strerror(err));
		return err;
	}

	addr_cell_len = get_cells_len(fdt, "#address-cells");
	size_cell_len = get_cells_len(fdt, "#size-cells");

	for_each_memory_bank(bank) {
		write_cell(tmp + len, bank->start, addr_cell_len);
		len += addr_cell_len;
		write_cell(tmp + len, bank->size, size_cell_len);
		len += size_cell_len;
	}

	err = fdt_setprop(fdt, nodeoffset, "reg", tmp, len);
	if (err < 0) {
		printf("WARNING: could not set %s %s.\n",
				"reg", fdt_strerror(err));
		return err;
	}
	return 0;
}

static int of_register_memory_fixup(void)
{
	return of_register_fixup(of_memory_fixup);
}
late_initcall(of_register_memory_fixup);
#endif
