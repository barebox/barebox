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

static int bthread_time(void)
{
	uint64_t start = get_time_ns();
	int i = 0;

	/*
	 * How many background tasks can we have in one second?
	 *
	 * A low number here may point to problems with bthreads taking too
	 * much time.
	 */
	while (!is_timeout(start, SECOND))
		i++;

	return i;
}

static int bthread_infinite(void *data)
{
	while (!bthread_should_stop())
		;

	return 0;
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

	bthread_stop(bthread);
	bthread_free(bthread);

	return i;
}

static int bthread_printer(void *arg)
{
	volatile u64 start;
	volatile unsigned long i = 0;
	start = get_time_ns();

	while (!bthread_should_stop()) {
		if (!is_timeout_non_interruptible(start, 225 * MSECOND))
			continue;

		if ((unsigned long)arg == i++)
			printf("%s yield #%lu\n", bthread_name(current), i);
		start = get_time_ns();
	}

	return i;
}

static int yields;

static int bthread_spawner(void *arg)
{
	struct bthread *bthread[4];
	volatile u64 start;
	volatile unsigned long i = 0;
	int ret = 0;
	int ecode;

	start = get_time_ns();

	for (i = 0; i < ARRAY_SIZE(bthread); i++) {
		bthread[i] = bthread_run(bthread_printer, (void *)(long)i,
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
		ecode = bthread_stop(bthread[i]);
		bthread_free(bthread[i]);

		if (!ret && (ecode != 4 || yields < ecode))
			ret = -EIO;
	}

	return ret;
}

struct spawn {
	struct bthread *bthread;
	struct list_head list;
};

static int do_bthread(int argc, char *argv[])
{
	LIST_HEAD(spawners);
	struct spawn *spawner, *tmp;
	int ret = 0;
	int ecode, opt, i = 0;
	bool time = false;

	while ((opt = getopt(argc, argv, "itcv")) > 0) {
		switch (opt) {
		case 'i':
			bthread_info();
			break;
		case 'c':
			yields = bthread_isolated_time();
			printf("%d bthread context switches possible in 1s\n", yields);
			break;
		case 'v':
			spawner = xzalloc(sizeof(*spawner));
			spawner->bthread = bthread_run(bthread_spawner, NULL,
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
		ecode = bthread_stop(spawner->bthread);
		bthread_free(spawner->bthread);
		if (!ret && ecode)
			ret = ecode;
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
	BAREBOX_CMD_HELP_OPT ("-v", "verify correct bthread operation")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bthread)
	.cmd = do_bthread,
	BAREBOX_CMD_DESC("print info about registered bthreads")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_bthread_help)
BAREBOX_CMD_END
