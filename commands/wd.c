// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2012 Juergen Beisert <kernel@pengutronix.de>

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <getopt.h>
#include <complete.h>
#include <watchdog.h>

/* default timeout in [sec] */
static unsigned timeout = CONFIG_CMD_WD_DEFAULT_TIMOUT;

static int do_wd(int argc, char *argv[])
{
	struct watchdog *wd = watchdog_get_default();
	int opt;
	int rc;
	bool do_ping = false;

	while ((opt = getopt(argc, argv, "d:xp")) > 0) {
		switch (opt) {
		case 'd':
			wd = watchdog_get_by_name(optarg);
			break;
		case 'x':
			return watchdog_inhibit_all();
		case 'p':
			do_ping = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind < argc) {
		if (isdigit(*argv[optind])) {
			timeout = simple_strtoul(argv[optind], NULL, 0);
		} else {
			printf("numerical parameter expected\n");
			return COMMAND_ERROR_USAGE;
		}
	}

	if (do_ping) {
		rc = watchdog_ping(wd);
		if (rc) {
			printf("watchdog ping failed: %pe (%d)\n", ERR_PTR(rc), rc);
			return COMMAND_ERROR;
		}

		return 0;
	}

	rc = watchdog_set_timeout(wd, timeout);
	if (rc < 0) {
		switch (rc) {
		case -EINVAL:
			printf("Timeout value out of range\n");
			break;
		case -ENOSYS:
			printf("Watchdog cannot be disabled\n");
			break;
		case -ENODEV:
			printf("Watchdog device doesn't exist.\n");
			break;
		default:
			printf("Watchdog fails: '%pe'\n", ERR_PTR(rc));
			break;
		}

		return COMMAND_ERROR;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(wd)
BAREBOX_CMD_HELP_TEXT("Enable the watchdog to bark in TIME seconds.")
BAREBOX_CMD_HELP_TEXT("When TIME is 0, the watchdog gets disabled,")
BAREBOX_CMD_HELP_TEXT("Without a parameter the watchdog will be re-triggered.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-d DEVICE\t", "watchdog name (default is highest priority watchdog)")
BAREBOX_CMD_HELP_OPT("-p\t", "ping watchdog")
BAREBOX_CMD_HELP_OPT("-x\t", "inhibit all watchdogs (i.e. disable or autopoll if possible)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(wd)
	.cmd = do_wd,
	BAREBOX_CMD_DESC("enable/disable/trigger the watchdog")
	BAREBOX_CMD_OPTS("[-d DEVICE] [-x] [TIME]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_wd_help)
	BAREBOX_CMD_COMPLETE(device_complete)
BAREBOX_CMD_END
