#ifndef __WORK_H
#define __WORK_H

#include <linux/list.h>

struct work_struct {
	struct list_head list;
};

struct work_queue {
	void (*fn)(struct work_struct *work);
	void (*cancel)(struct work_struct *work);

	struct list_head list;
	struct list_head work;
};

static inline void wq_queue_work(struct work_queue *wq, struct work_struct *work)
{
	list_add_tail(&work->list, &wq->work);
}

void wq_register(struct work_queue *wq);
void wq_unregister(struct work_queue *wq);

void wq_do_all_works(void);
void wq_cancel_work(struct work_queue *wq);

#endif /* __WORK_H */
