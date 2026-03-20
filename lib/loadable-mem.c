// SPDX-License-Identifier: GPL-2.0-only

#include <loadable.h>
#include <memory.h>
#include <zero_page.h>
#include <xfuncs.h>

struct mem_loadable_priv {
	const void *start;
	size_t size;
};

static int mem_loadable_get_info(struct loadable *l, struct loadable_info *info)
{
	struct mem_loadable_priv *priv = l->priv;

	info->final_size = priv->size;

	return 0;
}

/**
 * mem_loadable_extract_into_buf - copy memory region to target address
 * @l: loadable representing a memory region
 * @load_addr: virtual address to copy data to
 * @size: size of buffer at load_addr
 * @offset: offset within the memory region to start copying from
 * @flags: A bitmask of OR-ed LOADABLE_EXTRACT_ flags
 *
 * Copies data from the memory region to the specified address.
 *
 * No decompression is performed - the data is copied as-is.
 * The caller must provide a valid address range; this function does not allocate
 * memory.
 *
 * Return: actual number of bytes copied on success, negative errno on error
 */
static ssize_t mem_loadable_extract_into_buf(struct loadable *l,
					     void *load_addr, size_t size,
					     loff_t offset,
					     unsigned flags)
{
	struct mem_loadable_priv *priv = l->priv;

	if (offset >= priv->size)
		return 0;
	if (!(flags & LOADABLE_EXTRACT_PARTIAL) && priv->size - offset > size)
		return -ENOSPC;

	size = min_t(size_t, priv->size - offset, size);

	if (unlikely(zero_page_contains((ulong)load_addr)))
		zero_page_memcpy(load_addr, priv->start + offset, size);
	else
		memcpy(load_addr, priv->start + offset, size);

	return size; /* Actual bytes read */
}

/**
 * mem_loadable_mmap - return pointer to memory region
 * @l: loadable representing a memory region
 * @size: on success, set to the size of the memory region
 *
 * Returns a direct pointer to the memory region without copying.
 *
 * No decompression is performed - the data is accessed as-is.
 *
 * Return: read-only pointer to the memory region, or NULL for zero-size
 */
static const void *mem_loadable_mmap(struct loadable *l, size_t *size)
{
	struct mem_loadable_priv *priv = l->priv;

	*size = priv->size;
	return priv->start;
}

static void mem_loadable_release(struct loadable *l)
{
	struct mem_loadable_priv *priv = l->priv;

	free(priv);
}

static const struct loadable_ops mem_loadable_ops = {
	.get_info = mem_loadable_get_info,
	.extract_into_buf = mem_loadable_extract_into_buf,
	.mmap = mem_loadable_mmap,
	.release = mem_loadable_release,
};

/**
 * loadable_from_mem - create a loadable from a memory region
 * @mem: pointer to the memory region
 * @size: size of the memory region in bytes
 * @type: type of loadable (LOADABLE_KERNEL, LOADABLE_INITRD, etc.)
 *
 * Creates a loadable structure that wraps access to an existing memory region.
 * The memory is accessed directly during extract or via mmap with no
 * decompression - it is used as-is.
 *
 * The created loadable must be freed with loadable_release() when done.
 * The memory region must remain valid for the lifetime of the loadable.
 *
 * Return: pointer to allocated loadable (never fails due to xzalloc)
 */
struct loadable *loadable_from_mem(const void *mem, size_t size,
				   enum loadable_type type)
{
	struct loadable *l;
	struct mem_loadable_priv *priv;

	l = xzalloc(sizeof(*l));
	priv = xzalloc(sizeof(*priv));

	priv->start = size ? mem : NULL;
	priv->size = size;

	l->name = xasprintf("Mem(%p, %#zx)", mem, size);
	l->type = type;
	l->ops = &mem_loadable_ops;
	l->priv = priv;
	loadable_init(l);

	return l;
}
