/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *	Copyright (C) 2002 by Concurrent Computer Corporation
 */

#include <errno.h>
#include <linux/idr.h>

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

static int idr_compare(struct list_head *a, struct list_head *b)
{
	int id_a = list_entry(a, struct idr, list)->id;
	int id_b = list_entry(b, struct idr, list)->id;

	return __compare3(id_a, id_b);
}

int idr_alloc_one(struct idr *head, void *ptr, int start)
{
	struct idr *idr;

	if (__idr_find(head, start))
		return -EBUSY;

	idr = malloc(sizeof(*idr));
	if (!idr)
		return -ENOMEM;

	idr->id = start;
	idr->ptr = ptr;

	list_add_sort(&idr->list, &head->list, idr_compare);

	return start;
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
