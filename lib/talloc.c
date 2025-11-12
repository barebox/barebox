// SPDX-License-Identifier: GPL-2.0-only OR MIT
/*
 * Talloc is a replacement for the standard memory allocation routines that
 * provides structure aware allocations.
 *
 * @author Dario Sneidermanis
 *
 * Each chunk of talloc'ed memory has a header of the following form:
 *
 * +---------+---------+---------+--------···
 * |  first  |  next   |  prev   | memory
 * |  child  | sibling | sibling | chunk
 * +---------+---------+---------+--------···
 *
 * Thus, a talloc hierarchy tree would look like this:
 *
 *   NULL <-- chunk --> NULL
 *              ^
 *              |
 *              +-> chunk <--> chunk <--> chunk --> NULL
 *                    |          |          ^
 *                    v          v          |
 *                   NULL       NULL        +-> chunk <--> chunk --> NULL
 *                                                |          |
 *                                                v          v
 *                                               NULL       NULL
 */

#include <stdlib.h>
#include <string.h>
#include <linux/bug.h>
#include <linux/export.h>
#include <linux/container_of.h>
#include <asm/sections.h>
#include "talloc.h"

// TODO difference between set_parent and steal?
// TODO use a bit from TLSF for extra safety to differentiate talloc
// TODO allow leaf nodes (that have no children to be destructors!!)
#define  is_root(hdr) (!(hdr)->prev)
#define is_first(hdr) ((hdr)->prev->next != (hdr))

static inline void *hdr2mem(struct talloc *hdr)
{
	return hdr ? &hdr[1] : NULL;
}

static inline struct talloc *mem2hdr(const void *mem)
{
	return mem ? (void *)mem - sizeof(struct talloc) : NULL;
}

/**
 * talloc_ctx_init() - Initialize a raw chunk of memory.
 *
 * @hdr: pointer to freshly allocated memory chunk.
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 *
 * Return: pointer to the allocated memory chunk, or NULL if there was an error.
 */
static void *talloc_ctx_init(struct talloc *hdr, const void *parent)
{
	void *mem;

	if (!hdr)
		return NULL;

	memset(hdr, 0, sizeof(*hdr));

	mem = hdr2mem(hdr);
	talloc_steal(parent, mem);
	return mem;
}

/**
 * talloc_usable_size() - Report the size of the tallocation
 *
 * @mem: pointer to previously talloc'ed memory chunk.
 *
 * Return: size of tallocation
 */
size_t talloc_usable_size(void *mem)
{
	struct talloc *hdr = mem2hdr(mem);

	return malloc_usable_size(hdr) - sizeof(struct talloc);
}
EXPORT_SYMBOL(talloc_usable_size);

/**
 * talloc_new() - Allocate a talloc object without user data
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 *
 * Return: pointer to the allocated object, or NULL if there was an error.
 */
void *talloc_new(const void *parent)
{
	return talloc_ctx_init(malloc(sizeof(struct talloc)), parent);
}
EXPORT_SYMBOL(talloc_new);

/**
 * talloc_size() - Allocate a (contiguous) memory chunk.
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 * @size: amount of memory requested (in bytes).
 *
 * Return: pointer to the allocated memory chunk, or NULL if there was an error.
 */
void *talloc_size(const void *parent, size_t size)
{
	if (!size)
		return ZERO_SIZE_PTR;

	return talloc_ctx_init(malloc(size + sizeof(struct talloc)), parent);
}
EXPORT_SYMBOL(talloc_size);

/**
 * talloc_strdup() - Duplicate a string
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 * @str: string to duplicate
 *
 * Return: pointer to the duplicated string, or NULL if there was an error.
 */
char *talloc_strdup(const void *parent, const char *str)
{
	size_t len = strlen(str) + 1;
	void *usr;

	usr = talloc_size(parent, len);
	if (!usr)
		return NULL;

	return memcpy(usr, str, len);
}
EXPORT_SYMBOL(talloc_strdup);

/**
 * talloc_strdup_const() - Duplicate a string if not read-only
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 * @size: amount of memory requested (in bytes).
 *
 * Return: pointer to the allocated memory chunk, or NULL if there was an error.
 */
const char *talloc_strdup_const(const void *parent, const char *str)
{
	if (is_barebox_rodata((ulong)str))
		return str;

	return talloc_strdup(parent, str);
}
EXPORT_SYMBOL(talloc_strdup_const);

/**
 * tzalloc() - Allocate a zeroed (contiguous) memory chunk.
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 * @size: amount of memory requested (in bytes).
 *
 * Return: pointer to the allocated memory chunk, or NULL if there was an error.
 */
void *talloc_zero_size(const void *parent, size_t size)
{
	if (!size)
		return ZERO_SIZE_PTR;

	return talloc_ctx_init(calloc(1, size + sizeof(struct talloc)), parent);
}
EXPORT_SYMBOL(talloc_zero_size);

/**
 * trealloc() - Modify the size of a talloc'ed memory chunk.
 *
 * @parent: parent to set if mem is NULL.
 * @mem: pointer to previously talloc'ed memory chunk.
 * @size: amount of memory requested (in bytes).
 *
 * Return: pointer to the allocated memory chunk, or NULL if there was an error.
 */
void *talloc_realloc_size(const void *parent, void *mem, size_t size)
{
	struct talloc *oldhdr, *hdr;
	void *newmem;

	if (!size) {
		talloc_free(mem);
		return ZERO_SIZE_PTR;
	}
	if (ZERO_OR_NULL_PTR(mem))
		mem = NULL;

	oldhdr = mem2hdr(mem);

	newmem = realloc(oldhdr, size + sizeof(*oldhdr));
	if (!oldhdr || !newmem)
		return talloc_ctx_init(newmem, parent);

	if (oldhdr == newmem)
		return mem;

	/* Buffer start address changed, update all references. */
	hdr = newmem;
	mem = hdr2mem(hdr);

	if (hdr->child)
		hdr->child->parent = hdr;

	if (!is_root(hdr)) {
		if (hdr->next)
			hdr->next->prev = hdr;

		if (hdr->prev->next == oldhdr)
			hdr->prev->next = hdr;
		if (hdr->parent->child == oldhdr)
			hdr->parent->child = hdr;
	}

	return mem;
}
EXPORT_SYMBOL(talloc_realloc_size);

/**
 * __tfree() - Deallocate all the descendants of given parent recursively.
 *
 * @hdr: pointer to previously talloc'ed memory chunk.
 */
static void __tfree(struct talloc *hdr)
{
	if (ZERO_OR_NULL_PTR(hdr))
		return;

	/* Fail if the tree hierarchy has cycles. */

	ASSERT(hdr->prev);
	hdr->prev = NULL;

	__tfree(hdr->child);
	__tfree(hdr->next);
	free(hdr);
}

/**
 * talloc_free() - Deallocate a talloc'ed memory chunk and all the chunks depending on it.
 *
 * @mem: pointer to previously talloc'ed memory chunk.
 */
void talloc_free(void *mem)
{
	struct talloc *hdr = mem2hdr(mem);

	if (ZERO_OR_NULL_PTR(mem))
		return;

	talloc_steal(NULL, mem);

	__tfree(hdr->child);

	free(hdr);
}
EXPORT_SYMBOL(talloc_free);

/**
 * talloc_free_const() - call talloc_free, unless read/only memory
 *
 * @mem: pointer to previously talloc'ed memory chunk.
 */
void talloc_free_const(const void *mem)
{
	if (is_barebox_rodata((ulong)mem))
		return;

	talloc_free((void *)mem);
}
EXPORT_SYMBOL(talloc_free_const);

/**
 * talloc_parent() - Get the parent of a talloc'ed memory chunk
 *
 * @mem: pointer to previously talloc'ed memory chunk.
 *
 * Return: pointer to the parent memory chunk it depends on (could be NULL).
 */
void *talloc_parent(const void *mem)
{
	struct talloc *hdr = mem2hdr(mem);

	if (ZERO_OR_NULL_PTR(hdr) || is_root(hdr))
		return NULL;

	while (!is_first(hdr))
		hdr = hdr->prev;

	return hdr2mem(hdr->parent);
}
EXPORT_SYMBOL(talloc_parent);

void mem_set_parent(struct talloc *hdr, struct talloc *parent_hdr)
{
	if (ZERO_OR_NULL_PTR(hdr))
		return;

	if (!is_root(hdr)) {
		/* Remove node from old tree. */
		if (hdr->next)
			hdr->next->prev = hdr->prev;

		if (!is_first(hdr))
			hdr->prev->next = hdr->next;
		else
			hdr->parent->child = hdr->next;
	}

	hdr->next = hdr->prev = NULL;

	if (parent_hdr) {
		/* Insert node into new tree. */
		if (parent_hdr->child) {
			hdr->next = parent_hdr->child;
			parent_hdr->child->prev = hdr;
		}

		hdr->parent = parent_hdr;
		parent_hdr->child = hdr;
	}
}
EXPORT_SYMBOL(mem_set_parent);

/**
 * talloc_steal() - Reparent talloc'ed memory chunk
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk depends, or NULL.
 * @mem: pointer to previously talloc'ed memory chunk.
 *
 * Change the parent of a talloc'ed memory chunk. This will affect the
 * dependencies of the entire subtree rooted at the given chunk.
 */
void talloc_steal(const void *parent, void *mem)
{
	struct talloc *hdr = mem2hdr(mem);
	struct talloc *parent_hdr = mem2hdr(parent);

	mem_set_parent(hdr, parent_hdr);
}
EXPORT_SYMBOL(talloc_steal);

/**
 * talloc_steal_children() - Remove a talloc'ed memory chunk from the dependency tree
 *
 * @parent: pointer to previously talloc'ed memory chunk from which this
 *          chunk's children will depend, or NULL.
 * @mem: pointer to previously talloc'ed memory chunk.
 *
 * Removing a talloc'ed memory chunk from the dependency tree takes care of its
 * children (they will depend on the specified parent).
 */
void talloc_steal_children(const void *parent, void *mem)
{
	struct talloc *hdr = mem2hdr(mem);
	struct talloc *parent_hdr = mem2hdr(parent);

	if (ZERO_OR_NULL_PTR(hdr))
		return;

	talloc_steal(NULL, mem);

	if (!hdr->child)
		return;

	if (parent_hdr) {
		/* Insert mem children in front of the list of parent children. */
		if (parent_hdr->child) {
			struct talloc *last = hdr->child;

			while (last->next)
				last = last->next;

			parent_hdr->child->prev = last;
			last->next = parent_hdr->child;
		}

		parent_hdr->child = hdr->child;
	}

	hdr->child->parent = parent_hdr;
	hdr->child = NULL;
}
EXPORT_SYMBOL(talloc_steal_children);

size_t talloc_total_blocks(const void *mem)
{
	struct talloc *hdr = mem2hdr(mem);
	size_t nblocks = 0;

	if (ZERO_OR_NULL_PTR(hdr))
		return 0;

	nblocks++;

	for (hdr = hdr->child; hdr; hdr = hdr->next)
		nblocks++;

	return nblocks;
}
EXPORT_SYMBOL(talloc_total_blocks);
