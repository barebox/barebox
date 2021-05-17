/*
 * show_progress.c - simple progress bar functions
 *
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <fs.h>
#include <progress.h>
#include <linux/math64.h>

#define HASHES_PER_LINE	64

static loff_t printed;
static loff_t progress_max;
static unsigned spin;

void show_progress(loff_t now)
{
	char spinchr[] = "\\|/-";

	if (now < 0) {
		printf("%c\b", spinchr[spin++ % (sizeof(spinchr) - 1)]);
		return;
	}

	if (progress_max && progress_max != FILESIZE_MAX) {
		uint64_t tmp = now * HASHES_PER_LINE;
		now = div64_u64(tmp, progress_max);
	}

	while (printed < now) {
		if (!(printed % HASHES_PER_LINE) && printed)
			printf("\n\t");
		printf("#");
		printed++;
	}
}

void init_progression_bar(loff_t max)
{
	printed = 0;
	progress_max = max;
	spin = 0;
	if (progress_max && progress_max != FILESIZE_MAX)
		printf("\t[%*s]\r\t[", HASHES_PER_LINE, "");
	else
		printf("\t");
}

NOTIFIER_HEAD(progress_notifier_list);

static int progress_logger(struct notifier_block *r, unsigned long stage, void *_prefix)
{
	const char *prefix = _prefix;

	switch ((enum progress_stage)stage) {
		case PROGRESS_UPDATING:
			pr_info("%sSoftware update in progress\n", prefix);
			break;
		case PROGRESS_UPDATE_SUCCESS:
			pr_info("%sSoftware update finished successfully\n", prefix);
			break;
		case PROGRESS_UPDATE_FAIL:
			pr_info("%sSoftware update failed\n", prefix);
			break;
		case PROGRESS_UNSPECIFIED:
			/* default state. This should not be reached */
			break;
	}

	return 0;
}

struct notifier_block progress_log_client =  {
	.notifier_call = progress_logger
};
