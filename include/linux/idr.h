/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * include/linux/idr.h
 *
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *	Copyright (C) 2002 by Concurrent Computer Corporation
 *
 * Small id to pointer translation service avoiding fixed sized
 * tables.
 */

#ifndef __IDR_H__
#define __IDR_H__

#include <errno.h>
#include <linux/list.h>

struct idr {
	int			id;
	void			*ptr;
	struct list_head	list;
};

#define DEFINE_IDR(name)	\
	struct idr name = { .list = LIST_HEAD_INIT((name).list) }

/**
 * idr_for_each_entry() - Iterate over an IDR's elements of a given type.
 * @_idr: IDR handle.
 * @_entry: The type * to use as cursor
 * @_id: Entry ID.
 *
 * @_entry and @_id do not need to be initialized before the loop, and
 * after normal termination @_entry is left with the value NULL.  This
 * is convenient for a "not found" value.
 */
#define idr_for_each_entry(_idr, _entry, _id)				\
	for (struct idr *iter =						\
	     list_first_entry_or_null(&(_idr)->list, struct idr, list);	\
	     (iter && iter != (_idr)) || (_entry = NULL);	\
	     iter = list_next_entry(iter, list))			\
	if ((_entry = iter->ptr, _id = iter->id, true))

struct idr *__idr_find(struct idr *head, int lookup_id);

int idr_for_each(const struct idr *idr,
		 int (*fn)(int id, void *p, void *data), void *data);

static inline int idr_is_empty(const struct idr *idr)
{
	return list_empty(&idr->list);
}

static inline void *idr_find(struct idr *head, int id)
{
	struct idr *idr = __idr_find(head, id);

	return idr ? idr->ptr : NULL;
}

int idr_alloc_one(struct idr *head, void *ptr, int start);

static inline void idr_init(struct idr *idr)
{
	INIT_LIST_HEAD(&idr->list);
}

void idr_destroy(struct idr *idr);

void idr_remove(struct idr *idr, int id);

#endif /* __IDR_H__ */
