// SPDX-License-Identifier: GPL-2.0-only

#include <loadable.h>
#include <malloc.h>
#include <memory.h>
#include <libfile.h>
#include <unistd.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/sizes.h>
#include <zero_page.h>
#include <fs.h>

void loadable_set_name(struct loadable *l, const char *fmt, ...)
{
	va_list args;

	free(l->name);

	va_start(args, fmt);
	l->name = xvasprintf(fmt, args);
	va_end(args);
}

const char *loadable_type_tostr(enum loadable_type type)
{
	switch (type) {
	case LOADABLE_KERNEL:
		return "kernel";
	case LOADABLE_INITRD:
		return "initrd";
	case LOADABLE_FDT:
		return "fdt";
	case LOADABLE_TEE:
		return "tee";
	default:
		return NULL;
	}
}

/**
 * loadable_get_info - obtain metadata without loading data
 * @l: loadable
 * @info: loadable info populated on success
 *
 * This function reads the minimum necessary headers/properties
 * needed to determine size and addresses.
 * Result is cached in loadable->info.
 *
 * Return: 0 on success, or negative error code otherwise
 */
int loadable_get_info(struct loadable *l, struct loadable_info *info)
{
	int ret;

	if (!l->info_valid) {
		ret = l->ops->get_info(l, &l->info);
		if (ret)
			return ret;
		l->info_valid = true;
	}

	*info = l->info;
	return 0;
}

/**
 * loadable_extract_into_buf - load fully to target address
 * @l: loadable
 * @load_addr: virtual address to load data to
 * @size: size of buffer at load_addr
 * @offset: offset within the loadable to start loading from
 * @flags: A bitmask of OR-ed LOADABLE_EXTRACT_ flags
 *
 * Instructs the loadable implementation to populate the specified
 * buffer with data, so it can be reused. This may involve decompression
 * if the loadable was constructed by loadable_decompress for example.
 *
 * if !(@flags & LOADABLE_EXTRACT_PARTIAL), -ENOSPC will be returned if the
 * size is too small.
 *
 * Return: actual number of bytes read on success, negative errno on error
 */
ssize_t loadable_extract_into_buf(struct loadable *l, void *load_addr,
				  size_t size, loff_t offset, unsigned flags)
{
	return l->ops->extract_into_buf(l, load_addr, size, offset, flags);
}

#define EXTRACT_FD_BUF_SIZE	SZ_1M

/**
 * loadable_extract_into_fd - extract loadable data to a file descriptor
 * @l: loadable
 * @fd: file descriptor to write to
 *
 * Extracts data from the loadable and writes it to the specified file
 * descriptor. Uses a 1MB buffer and streams data in chunks.
 *
 * Return: total bytes written on success, negative errno on error
 */
ssize_t loadable_extract_into_fd(struct loadable *l, int fd)
{
	void *buf;
	loff_t offset = 0;
	ssize_t total = 0;
	ssize_t ret;

	buf = malloc(EXTRACT_FD_BUF_SIZE);
	if (!buf)
		return -ENOMEM;

	while (1) {
		ret = loadable_extract_into_buf(l, buf, EXTRACT_FD_BUF_SIZE,
						offset, LOADABLE_EXTRACT_PARTIAL);
		if (ret < 0)
			goto out;
		if (ret == 0)
			break;

		ret = write_full(fd, buf, ret);
		if (ret < 0)
			goto out;

		offset += ret;
		total += ret;
	}

	ret = total;
out:
	free(buf);
	return ret;
}

static ssize_t __loadable_extract_into_sdram(struct loadable *l,
					     unsigned long *adr,
					     unsigned long *size,
					     unsigned long end)
{
	ssize_t nbyts = loadable_extract_into_buf_full(l, (void *)*adr, *size);
	if (nbyts < 0)
		return nbyts;

	*adr += nbyts;
	*size -= nbyts;

	if (*adr - 1 > end)
		return -EBUSY;

	return nbyts;
}

/**
 * loadable_extract_into_sdram_all - extract main and chained loadables into SDRAM
 * @main: main loadable with optional chained loadables
 * @adr: start address of the SDRAM region
 * @end: end address of the SDRAM region (inclusive)
 *
 * Extracts the main loadable and all chained loadables into a contiguous SDRAM
 * region. Registers the region with the appropriate memory type based on
 * the loadable type.
 *
 * Return: SDRAM resource on success, or error pointer on failure
 */
struct resource *loadable_extract_into_sdram_all(struct loadable *main,
						 unsigned long adr,
						 unsigned long end)
{
	struct loadable *lc;
	unsigned long adr_orig = adr;
	unsigned long size = end - adr + 1;
	struct resource *res;
	unsigned memtype;
	unsigned memattrs;
	ssize_t ret;

	/* FIXME: EFI payloads are started with MMU enabled, so for now
	 * we keep attributes as RWX instead of remapping later on
	 */
	memattrs = IS_ENABLED(CONFIG_EFI_LOADER) ? MEMATTRS_RWX : MEMATTRS_RW;

	if (main->type == LOADABLE_KERNEL)
		memtype = MEMTYPE_LOADER_CODE;
	else
		memtype = MEMTYPE_LOADER_DATA;

	res = request_sdram_region(loadable_type_tostr(main->type) ?: "image",
				   adr, size, memtype, memattrs);
	if (!res)
		return ERR_PTR(-EBUSY);

	ret = __loadable_extract_into_sdram(main, &adr, &size, end);
	if (ret < 0)
		goto err;

	list_for_each_entry(lc, &main->chained_loadables, list) {
		ret = __loadable_extract_into_sdram(lc, &adr, &size, end);
		if (ret < 0)
			goto err;
	}

	if (adr == adr_orig) {
		release_region(res);
		return NULL;
	}

	ret = resize_region(res, adr - adr_orig);
	if (ret)
		goto err;

	return res;
err:
	release_sdram_region(res);
	return ERR_PTR(ret);
}

/**
 * loadable_mmap - memory map a buffer
 * @l: loadable
 * @size: size of memory map on success
 *
 * This is the most efficient way to access a loadable if supported.
 * The returned pointer should be passed to loadable_munmap when no
 * longer needed.
 *
 * Return: read-only pointer to the buffer, NULL for zero-size, or MAP_FAILED on error
 */
const void *loadable_mmap(struct loadable *l, size_t *size)
{
	return l->ops->mmap ? l->ops->mmap(l, size) : MAP_FAILED;
}

/**
 * loadable_munmap - unmap a buffer
 * @l: loadable
 * @buf: buffer returned from loadable_mmap
 * @size: size of memory map returned by loadable_mmap
 */
void loadable_munmap(struct loadable *l, const void *buf, size_t size)
{
	if (l->ops->munmap)
		l->ops->munmap(l, buf, size);
}

/**
 * loadable_extract - extract loadable into newly allocated buffer
 * @l: loadable
 * @size: on success, set to the number of bytes extracted
 *
 * Extracts the loadable data into a newly allocated buffer. The caller
 * is responsible for freeing the returned buffer with free().
 *
 * Return: allocated buffer, NULL for zero-size, or error pointer on failure
 */
void *loadable_extract(struct loadable *l, size_t *size)
{
	struct loadable_info li;
	ssize_t nbytes;
	void *buf;
	int ret;

	ret = loadable_get_info(l, &li);
	if (ret)
		return ERR_PTR(ret);

	if (li.final_size == LOADABLE_SIZE_UNKNOWN) {
		if (l->ops->extract)
			return l->ops->extract(l, size);
		if (l->ops->mmap) {
			const void *map = l->ops->mmap(l, size);
			if (map != MAP_FAILED) {
				void *dup = memdup(map, *size);
				if (l->ops->munmap)
					l->ops->munmap(l, map, *size);
				return dup;
			}
		}

		return ERR_PTR(-EFBIG);
	}

	/* We assume extract_into_buf to be more efficient as it
	 * can allocate once instead of having to resize
	 */
	buf = malloc(li.final_size);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	/* We are reading for the full size of the file, so set
	 * LOADABLE_EXTRACT_PARTIAL to save the underlying loadable
	 * some work
	 */
	nbytes = loadable_extract_into_buf(l, buf, li.final_size, 0,
					   LOADABLE_EXTRACT_PARTIAL);
	if (nbytes < 0) {
		free(buf);
		return ERR_PTR(nbytes);
	} else if (nbytes == 0) {
		free(buf);
		buf = NULL;
	}

	*size = nbytes;
	return buf;
}

/**
 * loadable_view - get read-only view of loadable
 * @l: loadable
 * @size: on success, set to the size of the view
 *
 * Use this when you want to non-destructively access the data.
 * It tries to find the optimal way to provide the buffer:
 * - If memory-mappable, return memory map without allocation
 * - If size is known, allocate once and use extract_into_buf
 * - If size is unknown, fall back to extract (assumed to be the slowest)
 *
 * Return: read-only pointer to the buffer, NULL for zero-size, or error pointer on failure
 */
const void *loadable_view(struct loadable *l, size_t *size)
{
	const void *mmap;

	mmap = loadable_mmap(l, size);
	if (mmap != MAP_FAILED) {
		l->mmap_active = true;
		return mmap;
	}

	l->mmap_active = false;
	return loadable_extract(l, size);
}

/**
 * loadable_view_free - free buffer returned by loadable_view()
 * @l: loadable
 * @buf: buffer returned by loadable_view()
 * @size: size returned by loadable_view()
 *
 * Releases resources associated with a buffer obtained via loadable_view().
 */
void loadable_view_free(struct loadable *l, const void *buf, size_t size)
{
	if (IS_ERR_OR_NULL(buf))
		return;

	if (l->mmap_active)
		loadable_munmap(l, buf, size);
	else
		free((void *)buf);
}

/**
 * loadable_chain - chain a loadable to another loadable
 * @main: pointer to the main loadable (may be NULL initially)
 * @new: loadable to chain
 *
 * Links @new to @main. If @main is NULL, @new becomes the new main.
 * Linked loadables are extracted together by loadable_extract_into_sdram_all().
 */
void loadable_chain(struct loadable **main, struct loadable *new)
{
	if (!new)
		return;
	if (!*main) {
		*main = new;
		return;
	}

	list_add_tail(&new->list, &(*main)->chained_loadables);
	list_splice_tail_init(&new->chained_loadables,
			      &(*main)->chained_loadables);
}

/**
 * loadable_release - free resources associated with this loadable
 * @lp: pointer to loadable (set to NULL on return)
 *
 * Release resources associated with a loadable and all chained loadables.
 *
 * This function is a no-op when passed NULL or error pointers.
 */
void loadable_release(struct loadable **lp)
{
	struct loadable *lc, *l, *tmp;

	if (IS_ERR_OR_NULL(lp) || IS_ERR_OR_NULL(*lp))
		return;

	l = *lp;

	list_for_each_entry_safe(lc, tmp, &l->chained_loadables, list)
		loadable_release(&lc);

	if (l->ops->release)
		l->ops->release(l);

	list_del(&l->list);
	free(l->name);
	free(l);
	*lp = NULL;
}
