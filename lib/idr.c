/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *	Copyright (C) 2002 by Concurrent Computer Corporation
 */

#include <errno.h>
#include <linux/idr.h>
#include <linux/bug.h>
#include <linux/limits.h>
#include <linux/minmax.h>
#include <xfuncs.h>

struct idr *__idr_find(struct idr *head, int lookup_id)
{
	struct idr *cursor;

	list_for_each_entry(cursor, &head->list, list) {
		if (cursor->id == lookup_id)
			return cursor;
	}

	return NULL;
}

/**
 * idr_for_each() - Iterate through all stored pointers.
 * @idr: IDR handle.
 * @fn: Function to be called for each pointer.
 * @data: Data passed to callback function.
 *
 * The callback function will be called for each entry in @idr, passing
 * the ID, the entry and @data.
 *
 * If @fn returns anything other than %0, the iteration stops and that
 * value is returned from this function.
 */
int idr_for_each(const struct idr *idr,
		 int (*fn)(int id, void *p, void *data), void *data)
{
	const struct idr *pos, *tmp;
	int ret;

	list_for_each_entry_safe(pos, tmp, &idr->list, list) {
		ret = fn(pos->id, pos->ptr, data);
		if (ret)
			return ret;
	}

	return 0;
}

static struct idr *__idr_alloc(void *ptr, u32 start)
{
	struct idr *idr;

	idr = xmalloc(sizeof(*idr));
	idr->id = start;
	idr->ptr = ptr;

	return idr;
}

/**
 * idr_alloc_u32() - Allocate an ID.
 * @head: IDR handle.
 * @ptr: Pointer to be associated with the new ID.
 * @nextid: Pointer to an ID.
 * @max: The maximum ID to allocate (inclusive).
 * @gfp: Memory allocation flags.
 *
 * Allocates an unused ID in the range specified by @nextid and @max.
 * Note that @max is inclusive whereas the @end parameter to idr_alloc()
 * is exclusive. The new ID is assigned to @nextid before the pointer
 * is inserted into the IDR.
 *
 * Return: 0 if an ID was allocated, -ENOMEM if memory allocation failed,
 * or -ENOSPC if no free IDs could be found.  If an error occurred,
 * @nextid is unchanged.
 */
int idr_alloc_u32(struct idr *head, void *ptr, u32 *nextid,
		  unsigned long max, gfp_t gfp)
{
	struct idr *idr, *prev, *next;

	if (list_empty(&head->list)) {
		idr = __idr_alloc(ptr, *nextid);
		list_add_tail(&idr->list, &head->list);
		return 0;
	}

	next = list_first_entry(&head->list, struct idr, list);
	if (*nextid < next->id) {
		idr = __idr_alloc(ptr, *nextid);
		list_add(&idr->list, &head->list);
		return 0;
	}

	prev = list_last_entry(&head->list, struct idr, list);
	if (prev->id < max) {
		*nextid = max_t(u32, prev->id + 1, *nextid);
		idr = __idr_alloc(ptr, *nextid);
		list_add_tail(&idr->list, &head->list);
		return 0;
	}

	list_for_each_entry_safe(prev, next, &head->list, list) {
		/* at the end of the list */
		if (&next->list == &head->list)
			break;

		if (prev->id > max)
			break;
		if (next->id < *nextid)
			continue;

		*nextid = max_t(u32, prev->id + 1, *nextid);
		idr = __idr_alloc(ptr, *nextid);
		list_add(&idr->list, &prev->list);
		return 0;
	}

	return -ENOSPC;
}

/**
 * idr_alloc() - Allocate an ID.
 * @head: IDR handle.
 * @ptr: Pointer to be associated with the new ID.
 * @start: The minimum ID (inclusive).
 * @end: The maximum ID (exclusive).
 * @gfp: Memory allocation flags.
 *
 * Allocates an unused ID in the range specified by @start and @end.  If
 * @end is <= 0, it is treated as one larger than %INT_MAX.  This allows
 * callers to use @start + N as @end as long as N is within integer range.
 *
 * Return: The newly allocated ID, -ENOMEM if memory allocation failed,
 * or -ENOSPC if no free IDs could be found.
 */
int idr_alloc(struct idr *head, void *ptr, int start, int end, gfp_t gfp)
{
	u32 id = start;
	int ret;

	if (WARN_ON_ONCE(start < 0))
		return -EINVAL;

	ret = idr_alloc_u32(head, ptr, &id, end > 0 ? end - 1 : INT_MAX, gfp);
	if (ret)
		return ret;

	return id;
}

static void __idr_remove(struct idr *idr)
{
	list_del(&idr->list);
	free(idr);
}

void idr_remove(struct idr *head, int id)
{
	struct idr *idr = __idr_find(head, id);
	if (!idr)
		return;

	__idr_remove(idr);
}

void idr_destroy(struct idr *idr)
{
	struct idr *pos, *tmp;

	if (!idr)
		return;

	list_for_each_entry_safe(pos, tmp, &idr->list, list)
		__idr_remove(pos);
}
