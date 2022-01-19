/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __WORK_H
#define __WORK_H

#include <linux/list.h>
#include <clock.h>

struct work_struct {
	struct list_head list;
	uint64_t timeout;
	bool delayed;
};

struct work_queue {
	void (*fn)(struct work_struct *work);
	void (*cancel)(struct work_struct *work);

	struct list_head list;
	struct list_head work;
};

static inline void wq_queue_work(struct work_queue *wq, struct work_struct *work)
{
	work->delayed = false;
	list_add_tail(&work->list, &wq->work);
}

static inline void wq_queue_delayed_work(struct work_queue *wq,
					 struct work_struct *work,
					 uint64_t delay_ns)
{
	work->timeout = get_time_ns() + delay_ns;
	work->delayed = true;
	list_add_tail(&work->list, &wq->work);
}

void wq_register(struct work_queue *wq);
void wq_unregister(struct work_queue *wq);

void wq_do_all_works(void);
void wq_cancel_work(struct work_queue *wq);

#endif /* __WORK_H */
