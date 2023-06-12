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

#define __idr_for_each_entry(head, idr) \
	list_for_each_entry((idr), &(head)->list, list)

static inline struct idr *__idr_find(struct idr *head, int id)
{
	struct idr *idr;

	__idr_for_each_entry(head, idr) {
		if (idr->id == id)
			return idr;
	}

	return NULL;
}

static inline void *idr_find(struct idr *head, int id)
{
	struct idr *idr = __idr_find(head, id);

	return idr ? idr->ptr : NULL;
}

static inline int idr_alloc_one(struct idr *head, void *ptr, int start)
{
	struct idr *idr;

	if (__idr_find(head, start))
		return -EBUSY;

	idr = malloc(sizeof(*idr));
	if (!idr)
		return -ENOMEM;

	idr->id = start;
	idr->ptr = ptr;

	list_add(&idr->list, &head->list);

	return start;
}

static inline void idr_init(struct idr *idr)
{
	INIT_LIST_HEAD(&idr->list);
}

static inline void idr_remove(struct idr *head, int id)
{
	struct idr *idr = __idr_find(head, id);

	list_del(&idr->list);
	free(idr);
}

#endif /* __IDR_H__ */
