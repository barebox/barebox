// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: 2016 Alexander Graf
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/aa703a816a62deb876a1e77ccff030a7cc60f344/lib/efi_loader/efi_memory.c

#define pr_fmt(fmt) "efi-loader: pool: " fmt

#include <efi/memory.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <linux/printk.h>
#include <linux/minmax.h>
#include <init.h>
#include <memory.h>
#include <linux/list_sort.h>
#include <linux/sizes.h>
#include <dma.h>

/* Magic number identifying memory allocated from pool */
#define EFI_ALLOC_POOL_MAGIC 0x1fe67ddf6491caa2

/**
 * struct efi_pool_allocation - memory block allocated from pool
 *
 * @num_pages:	number of pages allocated
 * @checksum:	checksum
 * @data:	allocated pool memory
 *
 * Each UEFI AllocatePool() request is serviced as a separate
 * (multiple) page allocation. We have to track the number of pages
 * to be able to free the correct amount later.
 *
 * The checksum calculated in function checksum() is used in FreePool() to avoid
 * freeing memory not allocated by AllocatePool() and duplicate freeing.
 *
 * EFI requires 8 byte alignment for pool allocations, so we can
 * prepend each allocation with these header fields.
 */
struct efi_pool_allocation {
	u64 num_pages;
	u64 checksum;
	char data[] __aligned(ARCH_DMA_MINALIGN);
};

/**
 * checksum() - calculate checksum for memory allocated from pool
 *
 * @alloc:	allocation header
 * Return:	checksum, always non-zero
 */
static u64 checksum(struct efi_pool_allocation *alloc)
{
	u64 addr = (uintptr_t)alloc;
	u64 ret = (addr >> 32) ^ (addr << 32) ^ alloc->num_pages ^
		  EFI_ALLOC_POOL_MAGIC;
	if (!ret)
		++ret;
	return ret;
}

/**
 * efi_allocate_pool - allocate memory from pool
 *
 * @pool_type:	type of the pool from which memory is to be allocated
 * @size:	number of bytes to be allocated
 * @buffer:	allocated memory
 * @name:	name for informational purposes
 * Return:	status code
 */
efi_status_t efi_allocate_pool(enum efi_memory_type pool_type, efi_uintn_t size,
			       void **buffer, const char *name)
{
	efi_status_t r;
	u64 addr;
	struct efi_pool_allocation *alloc;
	u64 num_pages = efi_size_in_pages(size +
					  sizeof(struct efi_pool_allocation));

	if (!buffer)
		return EFI_INVALID_PARAMETER;

	if (size == 0) {
		*buffer = NULL;
		return EFI_SUCCESS;
	}

	r = efi_allocate_pages(EFI_ALLOCATE_ANY_PAGES, pool_type, num_pages,
			       &addr, name);
	if (r == EFI_SUCCESS) {
		alloc = (struct efi_pool_allocation *)(uintptr_t)addr;
		alloc->num_pages = num_pages;
		alloc->checksum = checksum(alloc);
		*buffer = alloc->data;
	}

	return r;
}

/**
 * efi_alloc() - allocate boot services data pool memory
 *
 * Allocate memory from pool and zero it out.
 *
 * @size:	number of bytes to allocate
 * @name:		name for informational purposes
 * Return:	pointer to allocated memory or NULL
 */
void *efi_alloc(size_t size, const char *name)
{
	void *buf;

	if (efi_allocate_pool(EFI_BOOT_SERVICES_DATA, size, &buf, name) !=
	    EFI_SUCCESS) {
		pr_err("out of memory\n");
		return NULL;
	}
	memset(buf, 0, size);

	return buf;
}

/**
 * efi_realloc() - reallocate boot services data pool memory
 *
 * Reallocate memory from pool for a new size and copy the data from old one.
 *
 * @ptr:	pointer to old buffer
 * @size:	number of bytes to allocate
 * @name:	name for informational purposes
 * Return:	EFI status to indicate success or not
 */
efi_status_t efi_realloc(void **ptr, size_t size, const char *name)
{
	void *new_ptr;
	struct efi_pool_allocation *alloc;
	u64 num_pages = efi_size_in_pages(size +
					  sizeof(struct efi_pool_allocation));
	size_t old_size;

	if (!*ptr) {
		*ptr = efi_alloc(size, name);
		if (*ptr)
			return EFI_SUCCESS;
		return EFI_OUT_OF_RESOURCES;
	}

	alloc = container_of(*ptr, struct efi_pool_allocation, data);

	/* Check that this memory was allocated by efi_allocate_pool() */
	if (((uintptr_t)alloc & EFI_PAGE_MASK) ||
	    alloc->checksum != checksum(alloc)) {
		pr_err("%s: illegal realloc 0x%p\n", __func__, *ptr);
		return EFI_INVALID_PARAMETER;
	}

	/* Don't realloc. The actual size in pages is the same. */
	if (alloc->num_pages == num_pages)
		return EFI_SUCCESS;

	old_size = alloc->num_pages * EFI_PAGE_SIZE -
		sizeof(struct efi_pool_allocation);

	new_ptr = efi_alloc(size, name);
	if (!new_ptr)
		return EFI_OUT_OF_RESOURCES;

	/* copy old data to new alloced buffer */
	memcpy(new_ptr, *ptr, min(size, old_size));

	/* free the old buffer */
	efi_free_pool(*ptr);

	*ptr = new_ptr;

	return EFI_SUCCESS;
}

/**
 * efi_free_pool() - free memory from pool
 *
 * @buffer:	start of memory to be freed
 * Return:	status code
 */
efi_status_t efi_free_pool(void *buffer)
{
	efi_status_t ret;
	struct efi_pool_allocation *alloc;

	if (!buffer)
		return EFI_INVALID_PARAMETER;

	alloc = container_of(buffer, struct efi_pool_allocation, data);

	/* Check that this memory was allocated by efi_allocate_pool() */
	if (((uintptr_t)alloc & EFI_PAGE_MASK) ||
	    alloc->checksum != checksum(alloc)) {
		pr_err("%s: illegal free 0x%p\n", __func__, buffer);
		return EFI_INVALID_PARAMETER;
	}
	/* Avoid double free */
	alloc->checksum = 0;

	ret = efi_free_pages((uintptr_t)alloc, alloc->num_pages);

	return ret;
}
