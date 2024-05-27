// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2010 Marc Kleine-Budde <mkl@pengutronix.de>
 */

#define pr_fmt(fmt) "poller: " fmt

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <module.h>
#include <param.h>
#include <poller.h>
#include <clock.h>
#include <linux/ktime.h>

/*
 * Pollers are meant to poll and quickly execute actions.
 * Exceeding the maximum runtime below triggers a one-time warning.
 */
#define POLLER_MAX_RUNTIME_MS	20

static LIST_HEAD(poller_list);
static int __poller_active;

bool poller_active(void)
{
	return __poller_active;
}

int poller_register(struct poller_struct *poller, const char *name)
{
	if (poller->registered)
		return -EBUSY;

	poller->name = xstrdup(name);
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
	free(poller->name);

	return 0;
}

static void poller_async_callback(struct poller_struct *poller)
{
	struct poller_async *pa = container_of(poller, struct poller_async, poller);

	if (!pa->active)
		return;

	if (get_time_ns() < pa->end)
		return;

	pa->active = 0;
	pa->fn(pa->ctx);
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
	pa->active = 0;

	return 0;
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
	pa->end = get_time_ns() + delay_ns;
	pa->fn = fn;
	pa->active = 1;

	return 0;
}

int poller_async_register(struct poller_async *pa, const char *name)
{
	pa->poller.func = poller_async_callback;
	pa->active = 0;

	return poller_register(&pa->poller, name);
}

int poller_async_unregister(struct poller_async *pa)
{
	return poller_unregister(&pa->poller);
}

void poller_call(void)
{
	struct poller_struct *poller, *tmp;

	__poller_active = 1;

	list_for_each_entry_safe(poller, tmp, &poller_list, list) {
		ktime_t start = ktime_get();
		s64 duration_ms;

		poller->func(poller);

		duration_ms = ktime_ms_delta(ktime_get(), start);
		if (duration_ms > POLLER_MAX_RUNTIME_MS) {
			if (!poller->overtime)
				pr_warn("'%s' took unexpectedly long: %llums\n",
					poller->name, duration_ms);

			if (poller->overtime < U16_MAX)
				poller->overtime++;
		}
	}

	__poller_active = 0;
}

#if defined CONFIG_CMD_POLLER

#include <command.h>
#include <getopt.h>

static void poller_time(void)
{
	uint64_t start = get_time_ns();
	int i = 0;

	/*
	 * How many times we can run the registered pollers in one second?
	 *
	 * A low number here may point to problems with pollers taking too
	 * much time.
	 */
	while (!is_timeout(start, SECOND))
		i++;

	printf("%d poller calls in 1s\n", i);
}

static void poller_info(void)
{
	struct poller_struct *poller;

	printf("Registered pollers:\n");

	if (list_empty(&poller_list)) {
		printf("<none>\n");
		return;
	}

	list_for_each_entry(poller, &poller_list, list) {
		printf("%s", poller->name);
		if (poller->overtime)
			printf(": overtime %s%u",
			       poller->overtime == U16_MAX ? ">= " : "",
			       poller->overtime);
		printf("\n");
	}
}

BAREBOX_CMD_HELP_START(poller)
BAREBOX_CMD_HELP_TEXT("print info about registered pollers")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-i", "Print information about registered pollers")
BAREBOX_CMD_HELP_OPT ("-t", "measure how many pollers we run in 1s")
BAREBOX_CMD_HELP_END

static int do_poller(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "it")) > 0) {
		switch (opt) {
		case 'i':
			poller_info();
			return 0;
		case 't':
			poller_time();
			return 0;
		}
	}

	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_START(poller)
	.cmd = do_poller,
	BAREBOX_CMD_DESC("print info about registered pollers")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_poller_help)
BAREBOX_CMD_END
#endif
