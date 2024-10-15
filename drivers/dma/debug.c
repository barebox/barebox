/* SPDX-License-Identifier: GPL-2.0-only */

#include <dma.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/align.h>
#include <linux/kasan.h>
#include "debug.h"

/**
 * DMA_KASAN_ALIGN() - Align to KASAN granule size
 * @x:	value to be aligned
 *
 * kasan_unpoison_shadow will poison the bytes after it when the size
 * is not a multiple of the granule size. We thus always align the sizes
 * here. This allows unpoisoning a smaller buffer overlapping a bigger
 * buffer previously unpoisoned without KASAN detecting an out-of-bounds
 * memory access.
 *
 * A more accurate reflection of the hardware would be to align to
 * DMA_ALIGNMENT. We intentionally don't do this here as choosing the lowest
 * possible alignment allows us to catch more issues that may not pop up
 * when testing on platforms with higher DMA_ALIGNMENT.
 *
 * @returns the value after alignment
 */
#define DMA_KASAN_ALIGN(x)	ALIGN(x, 1 << KASAN_SHADOW_SCALE_SHIFT)

static LIST_HEAD(dma_mappings);

struct dma_debug_entry {
	struct list_head list;
	struct device    *dev;
	dma_addr_t       dev_addr;
	const void       *cpu_addr;
	size_t           size;
	int              direction;
};

static const char *dir2name[] = {
	[DMA_BIDIRECTIONAL] = "bidirectional",
	[DMA_TO_DEVICE] = "to-device",
	[DMA_FROM_DEVICE] = "from-device",
	[DMA_NONE] = "none",
};

#define dma_dev_printf(level, args...) do {	\
	if (level > LOGLEVEL)			\
		break;				\
	dev_printf((level), args);		\
	if ((level) <= MSG_WARNING)		\
		dump_stack();			\
} while (0)

#define dma_dev_warn(args...)	dma_dev_printf(MSG_WARNING, args)

static void dma_printf(int level, struct dma_debug_entry *entry,
		       const char *fmt, ...)
{
	struct va_format vaf;
	va_list va;

	va_start(va, fmt);

	vaf.fmt = fmt;
	vaf.va = &va;

	dma_dev_printf(level, entry->dev, "%s mapping 0x%llx+0x%zx: %pV\n",
		       dir2name[(entry)->direction], (u64)(entry)->dev_addr,
		       (entry)->size, &vaf);

	va_end(va);
}

#define dma_warn(args...)	dma_printf(MSG_WARNING, args)
#define dma_debug(args...)	dma_printf(MSG_DEBUG, args)

static inline int region_contains(struct dma_debug_entry *entry,
				  dma_addr_t buf_start, size_t buf_size)
{
	dma_addr_t dev_addr_end = entry->dev_addr + entry->size - 1;
	dma_addr_t buf_end = buf_start + buf_size - 1;

	/* Is the buffer completely within the mapping? */
	if (entry->dev_addr <= buf_start && dev_addr_end >= buf_end)
		return 1;

	/* Does the buffer partially overlap the mapping? */
	if (entry->dev_addr <= buf_end   && dev_addr_end >= buf_start)
		return -1;

	return 0;
}

static struct dma_debug_entry *
dma_debug_entry_find(struct device *dev, dma_addr_t dev_addr, size_t size)
{
	struct dma_debug_entry *entry;

	/*
	 * DMA functions should be called with a device argument to support
	 * non-1:1 device mappings.
	 */
	if (!dev)
		dma_dev_warn(NULL, "unportable NULL device passed with buffer 0x%llx+0x%zx!\n",
			     (u64)dev_addr, size);

	list_for_each_entry(entry, &dma_mappings, list) {
		if (dev != entry->dev)
			continue;

		switch (region_contains(entry, dev_addr, size)) {
		case 1:
			return entry;
		case -1:
			/* The same device shouldn't have two mappings for the same address */
			dma_warn(entry, "unexpected partial overlap looking for 0x%llx+0x%zx!\n",
				 (u64)dev_addr, size);
			fallthrough;
		case 0:
			continue;
		}
	}

	return NULL;
}

static const void *dma_debug_entry_cpu_addr(struct dma_debug_entry *entry,
				      dma_addr_t dma_handle)
{
	ptrdiff_t offset = dma_handle - entry->dev_addr;

	if (offset < 0)
		return NULL;

	return entry->cpu_addr + offset;
}

void debug_dma_map(struct device *dev, void *addr,
			  size_t size,
			  int direction, dma_addr_t dev_addr)
{
	struct dma_debug_entry *entry;

	entry = dma_debug_entry_find(dev, dev_addr, size);
	if (entry) {
		/* The same device shouldn't have two mappings for the same address */
		dma_warn(entry, "duplicate mapping\n");
		return;
	}

	entry = xmalloc(sizeof(*entry));

	entry->dev = dev;
	entry->dev_addr = dev_addr;
	entry->cpu_addr = addr;
	entry->size = size;
	entry->direction = direction;

	list_add(&entry->list, &dma_mappings);

	dma_debug(entry, "allocated\n");

	if (!IS_ALIGNED(dev_addr, DMA_ALIGNMENT))
		dma_dev_warn(dev, "Mapping insufficiently aligned %s buffer 0x%llx+0x%zx: %u bytes required!\n",
			     dir2name[direction], (u64)dev_addr, size, DMA_ALIGNMENT);

	kasan_poison_shadow(addr, DMA_KASAN_ALIGN(size), KASAN_DMA_DEV_MAPPED);
}

void debug_dma_unmap(struct device *dev, dma_addr_t addr,
		     size_t size, int direction)
{
	struct dma_debug_entry *entry;
	const void *cpu_addr;

	entry = dma_debug_entry_find(dev, addr, size);
	if (!entry) {
		/* Potential double free */
		dma_dev_warn(dev, "Unmapping non-mapped %s buffer 0x%llx+0x%zx!\n",
			     dir2name[direction], (u64)addr, size);
		return;
	}

	/* Mismatched size or direction may result in memory corruption */
	if (entry->size != size)
		dma_warn(entry, "mismatch unmapping 0x%zx bytes\n", size);
	if (entry->direction != direction)
		dma_warn(entry, "mismatch unmapping %s\n",
			 dir2name[direction]);

	dma_debug(entry, "deallocating\n");
	cpu_addr = dma_debug_entry_cpu_addr(entry, addr);
	if (cpu_addr)
		kasan_unpoison_shadow(cpu_addr, DMA_KASAN_ALIGN(size));
	list_del(&entry->list);
	free(entry);
}

void debug_dma_sync_single_for_cpu(struct device *dev,
				   dma_addr_t dma_handle, size_t size,
				   int direction)
{
	struct dma_debug_entry *entry;
	const void *cpu_addr;

	entry = dma_debug_entry_find(dev, dma_handle, size);
	if (!entry) {
		dma_dev_warn(dev, "sync for CPU of never-mapped %s buffer 0x%llx+0x%zx!\n",
			     dir2name[direction], (u64)dma_handle, size);
		return;
	}

	cpu_addr = dma_debug_entry_cpu_addr(entry, dma_handle);
	if (cpu_addr) {
		size_t kasan_size = DMA_KASAN_ALIGN(size);

		if (kasan_is_poisoned_shadow(cpu_addr, kasan_size) == 0) {
			dma_dev_warn(dev, "unexpected sync for CPU of already CPU-mapped %s buffer 0x%llx+0x%zx!\n",
				     dir2name[direction], (u64)dma_handle, size);
		} else {
			kasan_unpoison_shadow(cpu_addr, kasan_size);
		}
	}
}

void debug_dma_sync_single_for_device(struct device *dev,
				      dma_addr_t dma_handle,
				      size_t size, int direction)
{
	struct dma_debug_entry *entry;
	const void *cpu_addr;

	/*
	 * If dma_map_single was omitted, CPU cache may contain dirty cache lines
	 * for a buffer used for DMA. These lines may be evicted and written back
	 * after device DMA and before consumption by CPU, resulting in memory
	 * corruption
	 */
	entry = dma_debug_entry_find(dev, dma_handle, size);
	if (!entry) {
		dma_dev_warn(dev, "Syncing for device of never-mapped %s buffer 0x%llx+0x%zx!\n",
			     dir2name[direction], (u64)dma_handle, size);
		return;
	}

	cpu_addr = dma_debug_entry_cpu_addr(entry, dma_handle);
	if (cpu_addr) {
		size_t kasan_size = DMA_KASAN_ALIGN(size);

		/* explicit == 1 check as function may return negative value on error */
		if (kasan_is_poisoned_shadow(cpu_addr, kasan_size) == 1) {
			dma_dev_warn(dev, "unexpected sync for device of already device-mapped %s buffer 0x%llx+0x%zx!\n",
				     dir2name[direction], (u64)dma_handle, size);
		} else {
			kasan_poison_shadow(cpu_addr, kasan_size, KASAN_DMA_DEV_MAPPED);
		}

	}
}
