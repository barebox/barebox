/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <bthread.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <command.h>
#include <getopt.h>
#include <clock.h>
#include <slice.h>
#include <linux/kernel.h>

static int bthread_time(void)
{
	uint64_t start = get_time_ns();
	int i = 0;

	slice_release(&command_slice);

	/*
	 * How many background tasks can we have in one second?
	 *
	 * A low number here may point to problems with bthreads taking too
	 * much time.
	 */
	while (!is_timeout(start, SECOND))
		i++;

	slice_acquire(&command_slice);

	return i;
}

static void bthread_infinite(void *data)
{
	while (!bthread_should_stop())
		;
}

static int bthread_isolated_time(void)
{
	uint64_t start = get_time_ns();
	struct bthread *bthread;
	int i = 0;

	bthread = bthread_run(bthread_infinite, NULL, "infinite");
	if (!bthread)
		return -ENOMEM;

	/* main thread is the first in the run queue. Newly created bthread
	 * is the last. So if main_thread explicitly schedules new bthread,
	 * it will schedule back to main_thread afterwards and we won't
	 * execute any other threads in-between.
	 */

	while (!is_timeout_non_interruptible(start, SECOND)) {
		bthread_schedule(bthread);
		i += 2;
	}

	__bthread_stop(bthread);

	return i;
}

struct arg {
	unsigned long in;
	long out;
};

static void bthread_printer(void *_arg)
{
	struct arg *arg = _arg;
	volatile u64 start;
	volatile unsigned long i = 0;
	start = get_time_ns();

	while (!bthread_should_stop()) {
		if (!is_timeout_non_interruptible(start, 225 * MSECOND))
			continue;

		if (arg->in == i++)
			printf("%s yield #%lu\n", bthread_name(current), i);
		start = get_time_ns();
	}

	arg->out = i;
}

static int yields;

static void bthread_spawner(void *_spawner_arg)
{
	struct arg *arg, *spawner_arg = _spawner_arg;
	struct bthread *bthread[4];
	volatile u64 start;
	volatile unsigned long i = 0;
	int ret = 0;

	start = get_time_ns();

	for (i = 0; i < ARRAY_SIZE(bthread); i++) {
		arg = malloc(sizeof(*arg));
		arg->in = i;
		bthread[i] = bthread_run(bthread_printer, arg,
					 "%s-bthread%u", bthread_name(current), i+1);
		if (!bthread[i]) {
			ret = -ENOMEM;
			goto cleanup;
		}
	}

	while (!bthread_should_stop())
		;

cleanup:
	while (i--) {
		arg = bthread_data(bthread[i]);
		__bthread_stop(bthread[i]);

		if (!ret && (arg->out != 4 || yields < arg->out))
			ret = -EIO;
		free(arg);
	}

	spawner_arg->out = ret;
}

struct spawn {
	struct bthread *bthread;
	struct list_head list;
};

static int do_bthread(int argc, char *argv[])
{
	static int dummynr;
	static LIST_HEAD(dummies);
	LIST_HEAD(spawners);
	struct spawn *spawner, *tmp;
	int ret = 0;
	int opt, i = 0;
	bool time = false;
	struct arg *arg;

	while ((opt = getopt(argc, argv, "aritcv")) > 0) {
		switch (opt) {
		case 'a':
			spawner = xzalloc(sizeof(*spawner));
			spawner->bthread = bthread_run(bthread_infinite, NULL,
						       "dummy%u", dummynr++);
			if (!spawner->bthread) {
				free(spawner);
				ret = -ENOMEM;
				goto cleanup;
			}

			list_add(&spawner->list, &dummies);
			break;
		case 'r':
			if (dummynr == 0)
				return -EINVAL;
			spawner = list_first_entry(&dummies, struct spawn, list);
			bthread_cancel(spawner->bthread);
			list_del(&spawner->list);
			free(spawner);
			dummynr--;
			break;
		case 'i':
			bthread_info();
			break;
		case 'c':
			yields = bthread_isolated_time();
			printf("%d bthread context switches possible in 1s\n", yields);
			break;
		case 'v':
			spawner = xzalloc(sizeof(*spawner));
			arg = malloc(sizeof(*arg));
			spawner->bthread = bthread_run(bthread_spawner, arg,
						       "spawner%u", ++i);
			if (!spawner->bthread) {
				free(spawner);
				ret = -ENOMEM;
				goto cleanup;
			}

			/* We create intermediate spawning threads to test thread
			 * creation and scheduling from non-main thread.
			 */
			list_add(&spawner->list, &spawners);

			/* fallthrough */
		case 't':
			time = true;
		}
	}

	if (time) {
		yields = bthread_time();
		printf("%d bthread yield calls in 1s\n", yields);
	}

cleanup:
	list_for_each_entry_safe(spawner, tmp, &spawners, list) {
		arg = bthread_data(spawner->bthread);
		__bthread_stop(spawner->bthread);
		if (!ret && arg->out)
			ret = arg->out;
		free(arg);
		free(spawner);
	}

	return ret;
}

BAREBOX_CMD_HELP_START(bthread)
	BAREBOX_CMD_HELP_TEXT("print info about registered barebox threads")
	BAREBOX_CMD_HELP_TEXT("")
	BAREBOX_CMD_HELP_TEXT("Options:")
	BAREBOX_CMD_HELP_OPT ("-i", "Print information about registered bthreads")
	BAREBOX_CMD_HELP_OPT ("-t", "measure how many bthreads we currently run in 1s")
	BAREBOX_CMD_HELP_OPT ("-c", "count maximum context switches in 1s")
	BAREBOX_CMD_HELP_OPT ("-a", "add a dummy bthread")
	BAREBOX_CMD_HELP_OPT ("-r", "remove a dummy bthread")
	BAREBOX_CMD_HELP_OPT ("-v", "verify correct bthread operation")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bthread)
	.cmd = do_bthread,
	BAREBOX_CMD_DESC("print info about registered bthreads")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_bthread_help)
BAREBOX_CMD_END
