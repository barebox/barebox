/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <watchdog.h>

/* default timeout in [sec] */
static unsigned timeout = CONFIG_CMD_WD_DEFAULT_TIMOUT;

static int do_wd(int argc, char *argv[])
{
	int rc;

	if (argc > 1) {
		if (isdigit(*argv[1])) {
			timeout = simple_strtoul(argv[1], NULL, 0);
		} else {
			printf("numerical parameter expected\n");
			return 1;
		}
	}

	rc = watchdog_set_timeout(timeout);
	if (rc < 0) {
		switch (rc) {
		case -EINVAL:
			printf("Timeout value out of range\n");
			break;
		case -ENOSYS:
			printf("Watchdog cannot be disabled\n");
			break;
		default:
			printf("Watchdog fails: '%s'\n", strerror(-rc));
			break;
		}

		return 1;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(wd)
BAREBOX_CMD_HELP_USAGE("wd [<time>]\n")
BAREBOX_CMD_HELP_SHORT("enable the watchdog to bark in <time> seconds. "
		"When <time> is 0, the watchdog gets disabled,\n"
		"without a parameter the watchdog will be re-triggered\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(wd)
	.cmd = do_wd,
	.usage = "enable/disable/trigger the watchdog",
	BAREBOX_CMD_HELP(cmd_wd_help)
BAREBOX_CMD_END
