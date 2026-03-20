/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __LOADABLE_H
#define __LOADABLE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/limits.h>
#include <linux/bits.h>
#include <linux/fs.h>
#include <filetype.h>

struct loadable;

/**
 * enum loadable_type - type of boot component
 * @LOADABLE_UNSPECIFIED: unspecified type
 * @LOADABLE_KERNEL: kernel image
 * @LOADABLE_INITRD: initial ramdisk
 * @LOADABLE_FDT: flattened device tree
 * @LOADABLE_TEE: trusted execution environment
 */
enum loadable_type {
	LOADABLE_UNSPECIFIED,
	LOADABLE_KERNEL,
	LOADABLE_INITRD,
	LOADABLE_FDT,
	LOADABLE_TEE,
};

/**
 * loadable_type_tostr - convert loadable type enum to string
 * @type: loadable type to convert
 *
 * Return: string representation of the type, or NULL for unknown types
 */
const char *loadable_type_tostr(enum loadable_type type);

#define LOADABLE_SIZE_UNKNOWN		SIZE_MAX

/**
 * struct loadable_info - metadata about a loadable (no data loaded yet)
 * @final_size: final size in bytes, or LOADABLE_SIZE_UNKNOWN if unknown
 */
struct loadable_info {
	size_t final_size;
};

#define LOADABLE_EXTRACT_PARTIAL	BIT(0)

/**
 * struct loadable_ops - operations for a loadable
 */
struct loadable_ops {
	/**
	 * get_info - obtain metadata without loading data
	 *
	 * Must not allocate large buffers or decompress. Should read only
	 * headers/properties needed to determine size and addresses.
	 * Result is cached in loadable->info.
	 */
	int (*get_info)(struct loadable *l, struct loadable_info *info);

	/**
	 * extract_into_buf - load/decompress to target address
	 *
	 * @l: loadable
	 * @load_addr: final RAM address where data should reside
	 * @size: size of buffer at load_addr
	 * @offset: offset within the loadable to start loading from
	 * @flags: A bitmask of OR-ed LOADABLE_EXTRACT_ flags
	 *
	 * This is where data transfer happens.
	 * For compressed data: decompress to load_addr.
	 * For uncompressed data: read/copy to load_addr.
	 *
	 * Behavior:
	 *   - Must respect the provided load_addr
	 *   - Must check if buffer is sufficient, return -ENOSPC if too small
	 *     unless flags & LOADABLE_EXTRACT_PARTIAL.
	 *
	 * Returns: actual number of bytes written on success, negative errno on error
	 */
	ssize_t (*extract_into_buf)(struct loadable *l, void *load_addr,
				    size_t size, loff_t offset, unsigned flags);

	/**
	 * extract - load/decompress into newly allocated buffer
	 *
	 * @l: loadable
	 * @size: on successful return, *size is set to the number of bytes extracted
	 *
	 * Allocates a buffer and extracts loadable data into it. The buffer
	 * must be freed with free().
	 *
	 * Returns: allocated buffer, NULL for zero-size, or error pointer on failure
	 */
	void *(*extract)(struct loadable *l, size_t *size);

	/**
	 * mmap - memory map loaded/decompressed buffer
	 *
	 * @l: loadable
	 * @size: on successful return, *size is set to the size of the memory map
	 *
	 * Prepares a memory map of the loadable without copying data.
	 *
	 * Returns: pointer to mapped data, NULL for zero-size, or MAP_FAILED on error
	 */
	const void *(*mmap)(struct loadable *l, size_t *size);

	/**
	 * munmap - release mmap'ed region
	 *
	 * @l: loadable
	 * @buf: pointer returned by mmap
	 * @size: size returned by mmap
	 */
	void (*munmap)(struct loadable *l, const void *buf, size_t size);

	/**
	 * release - free resources associated with this loadable
	 *
	 * @l: loadable
	 *
	 * Called during cleanup to free implementation-specific resources.
	 */
	void (*release)(struct loadable *l);
};

/**
 * struct loadable - lazy-loadable boot component
 * @name: descriptive name for debugging
 * @type: type of component (kernel, initrd, fdt, tee)
 * @ops: operations for this loadable
 * @priv: format-specific private data
 * @info: cached metadata populated by get_info()
 * @info_valid: whether @info cache is valid
 * @mmap_active: whether an mmap is currently active
 * @chained_loadables: list of additional loadables chained to this one
 * @list: list node for chained_loadables
 *
 * Represents something that can be loaded to RAM (kernel, initrd, fdt, tee).
 * Metadata can be queried without loading. Actual loading happens on extract
 * or via mmap.
 */
struct loadable {
	char *name;
	enum loadable_type type;

	const struct loadable_ops *ops;
	void *priv;

	struct loadable_info info;
	bool info_valid;
	bool mmap_active;

	struct list_head chained_loadables;
	struct list_head list;
};

/**
 * loadable_init - initialize a loadable structure
 * @loadable: loadable to initialize
 *
 * Initializes list heads for a newly allocated loadable.
 */
static inline void loadable_init(struct loadable *loadable)
{
	INIT_LIST_HEAD(&loadable->chained_loadables);
	INIT_LIST_HEAD(&loadable->list);
}

void loadable_chain(struct loadable **main, struct loadable *new);

/**
 * loadable_count - count total loadables in chain
 * @l: main loadable (may be NULL or error pointer)
 *
 * Returns: number of loadables including main and all chained loadables
 */
static inline size_t loadable_count(struct loadable *l)
{
	if (IS_ERR_OR_NULL(l))
		return 0;
	return 1 + list_count_nodes(&l->chained_loadables);
}

/**
 * loadable_is_main - check if loadable is a main (non-chained) loadable
 * @l: loadable to check
 *
 * Returns: true if loadable is main, false otherwise
 */
static inline bool loadable_is_main(struct loadable *l)
{
	if (IS_ERR_OR_NULL(l))
		return false;
	return list_empty(&l->list);
}

/* Core API */

/**
 * loadable_set_name - set loadable name with printf-style formatting
 * @l: loadable
 * @fmt: printf-style format string
 * @...: format arguments
 *
 * Updates the name of the loadable. The old name is freed if present.
 */
void loadable_set_name(struct loadable *l, const char *fmt, ...) __printf(2, 3);
int loadable_get_info(struct loadable *l, struct loadable_info *info);

/**
 * loadable_get_size - get final size of loadable
 * @l: loadable
 * @size: on success, set to final size or FILE_SIZE_STREAM if unknown
 *
 * Returns: 0 on success, negative errno on error
 */
static inline int loadable_get_size(struct loadable *l, loff_t *size)
{
	struct loadable_info info;
	int ret = loadable_get_info(l, &info);
	if (ret)
		return ret;

	if (info.final_size == LOADABLE_SIZE_UNKNOWN)
		*size = FILE_SIZE_STREAM;
	else
		*size = info.final_size;

	return 0;
}

ssize_t loadable_extract_into_buf(struct loadable *l, void *load_addr,
				  size_t size, loff_t offset, unsigned flags);

static inline ssize_t loadable_extract_into_buf_full(struct loadable *l, void *load_addr,
						     size_t size)
{
	return loadable_extract_into_buf(l, load_addr, size, 0, 0);
}

ssize_t loadable_extract_into_fd(struct loadable *l, int fd);
void *loadable_extract(struct loadable *l, size_t *size);
const void *loadable_mmap(struct loadable *l, size_t *size);
const void *loadable_view(struct loadable *l, size_t *size);
void loadable_view_free(struct loadable *l, const void *buf, size_t size);
void loadable_munmap(struct loadable *l, const void *, size_t size);
struct resource *loadable_extract_into_sdram_all(struct loadable *l, unsigned long adr,
						 unsigned long end);

void loadable_release(struct loadable **l);

__returns_nonnull struct loadable *
loadable_from_mem(const void *mem, size_t size, enum loadable_type type);

__returns_nonnull struct loadable *
loadable_from_file(const char *path, enum loadable_type type);

int loadables_from_files(struct loadable **l,
			 const char *files, const char *delimiters,
			 enum loadable_type type);

#ifdef CONFIG_LOADABLE_DECOMPRESS
struct loadable *loadable_decompress(struct loadable *l);
#else
static inline struct loadable *loadable_decompress(struct loadable *l)
{
	return l;  /* Pass-through when not configured */
}
#endif

#endif /* __LOADABLE_H */
