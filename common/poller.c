/*
 * Copyright (C) 2010 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <module.h>
#include <param.h>
#include <poller.h>
#include <clock.h>

static LIST_HEAD(poller_list);
static int poller_active;

int poller_register(struct poller_struct *poller)
{
	if (poller->registered)
		return -EBUSY;

	list_add_tail(&poller->list, &poller_list);
	poller->registered = 1;

	return 0;
}

int poller_unregister(struct poller_struct *poller)
{
	if (!poller->registered)
		return -ENODEV;

	list_del(&poller->list);
	poller->registered = 0;

	return 0;
}

static void poller_async_callback(struct poller_struct *poller)
{
	struct poller_async *pa = container_of(poller, struct poller_async, poller);

	if (get_time_ns() < pa->end)
		return;

	pa->fn(pa->ctx);
	pa->fn = NULL;
	poller_unregister(&pa->poller);
}

/*
 * Cancel an outstanding asynchronous function call
 *
 * @pa		the poller that has been scheduled
 *
 * Cancel an outstanding function call. Returns 0 if the call
 * has actually been cancelled or -ENODEV when the call wasn't
 * scheduled.
 *
 */
int poller_async_cancel(struct poller_async *pa)
{
	return poller_unregister(&pa->poller);
}

/*
 * Call a function asynchronously
 *
 * @pa		the poller to be used
 * @delay	The delay in nanoseconds
 * @fn		The function to call
 * @ctx		context pointer passed to the function
 *
 * This calls the passed function after a delay of delay_ns. Returns
 * a pointer which can be used as a cookie to cancel a scheduled call.
 */
int poller_call_async(struct poller_async *pa, uint64_t delay_ns,
		void (*fn)(void *), void *ctx)
{
	pa->ctx = ctx;
	pa->poller.func = poller_async_callback;
	pa->end = get_time_ns() + delay_ns;
	pa->fn = fn;

	return poller_register(&pa->poller);
}

void poller_call(void)
{
	struct poller_struct *poller, *tmp;

	if (poller_active)
		return;

	poller_active = 1;

	list_for_each_entry_safe(poller, tmp, &poller_list, list)
		poller->func(poller);

	poller_active = 0;
}
