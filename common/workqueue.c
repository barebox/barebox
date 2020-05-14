// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <work.h>

static void wq_do_pending_work(struct work_queue *wq)
{
	struct work_struct *work, *tmp;

	list_for_each_entry_safe(work, tmp, &wq->work, list) {
		list_del(&work->list);
		wq->fn(work);
	}
}

void wq_cancel_work(struct work_queue *wq)
{
	struct work_struct *work, *tmp;

	list_for_each_entry_safe(work, tmp, &wq->work, list) {
		list_del(&work->list);
		wq->cancel(work);
	}
}

static LIST_HEAD(work_queues);

/**
 * wq_do_all_works - do all pending work
 *
 * This calls all pending work functions
 */
void wq_do_all_works(void)
{
	struct work_queue *wq;

	list_for_each_entry(wq, &work_queues, list)
		wq_do_pending_work(wq);
}

/**
 * wq_register - register a new work queue
 * @wq:    The work queue
 */
void wq_register(struct work_queue *wq)
{
	INIT_LIST_HEAD(&wq->work);
	list_add_tail(&wq->list, &work_queues);
}

/**
 * wq_unregister - unregister a work queue
 * @wq:    The work queue
 */
void wq_unregister(struct work_queue *wq)
{
	wq_cancel_work(wq);
	list_del(&wq->list);
}
