// SPDX-License-Identifier: GPL-2.0-only
/*
 * console_countdown - contdown on the console - interruptible by a keypress
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <clock.h>
#include <command.h>
#include <errno.h>
#include <console_countdown.h>
#include <stdio.h>
#include <readkey.h>

static bool console_countdown_timeout_abort;

void console_countdown_abort(const char *reason)
{
	if (reason && !console_countdown_timeout_abort)
		pr_info("\nCount down aborted by %s\n", reason);
	console_countdown_timeout_abort = true;
}

static int key_in_list(char key, const char *keys)
{
	if (!keys)
		return false;

	while (*keys) {
		if (key == *keys)
			return true;
		keys++;
	}

	return false;
}

int console_countdown(int timeout_s, unsigned flags, const char *keys,
		      char *out_key)
{
	uint64_t start, second;
	int countdown, ret = -EINTR;
	int key = 0;

	start = get_time_ns();
	second = start;

	countdown = timeout_s;

	if (!(flags & CONSOLE_COUNTDOWN_SILENT))
		printf("%4d", countdown--);

	do {
		if (tstc()) {
			key = getchar();
			if (key >= 0) {
				if (key_in_list(key, keys))
					goto out;
				if (flags & CONSOLE_COUNTDOWN_ANYKEY)
					goto out;
				if (flags & CONSOLE_COUNTDOWN_RETURN && (key == '\n' || key == '\r'))
					goto out;
				if (flags & CONSOLE_COUNTDOWN_CTRLC && key == CTL_CH('c'))
					goto out;
			}
			key = 0;
		}
		if ((flags & CONSOLE_COUNTDOWN_EXTERN) &&
		    console_countdown_timeout_abort)
			goto out;
		if (!(flags & CONSOLE_COUNTDOWN_SILENT) &&
		    is_timeout(second, SECOND)) {
			printf("\b\b\b\b%4d", countdown--);
			second += SECOND;
		}
	} while (!is_timeout(start, timeout_s * SECOND));

	if ((flags & CONSOLE_COUNTDOWN_EXTERN) &&
	    console_countdown_timeout_abort)
		goto out;

	ret = 0;

 out:
	if (!(flags & CONSOLE_COUNTDOWN_SILENT))
		printf("\n");
	if (key && out_key)
		*out_key = key;
	console_countdown_timeout_abort = false;

	return ret;
}
