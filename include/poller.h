/*
 * Copyright (C) 2010 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef POLLER_H
#define POLLER_H

#include <linux/list.h>

struct poller_struct {
	void (*func)(struct poller_struct *poller);
	int registered;
	struct list_head list;
};

int poller_register(struct poller_struct *poller);
int poller_unregister(struct poller_struct *poller);

struct poller_async;

struct poller_async {
	struct poller_struct poller;
	void (*fn)(void *);
	void *ctx;
	uint64_t end;
};

int poller_call_async(struct poller_async *pa, uint64_t delay_ns,
		void (*fn)(void *), void *ctx);
int poller_async_cancel(struct poller_async *pa);

#ifdef CONFIG_POLLER
void poller_call(void);
#else
static inline void poller_call(void)
{
}
#endif	/* CONFIG_POLLER */

#endif	/* !POLLER_H */
