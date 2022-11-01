// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <clock.h>
#include <getopt.h>
#include <linux/math64.h>

#define NSEC_PER_MINUTE	(NSEC_PER_SEC * 60LL)
#define NSEC_PER_HOUR	(NSEC_PER_MINUTE * 60LL)
#define NSEC_PER_DAY	(NSEC_PER_HOUR * 24LL)
#define NSEC_PER_WEEK	(NSEC_PER_DAY * 7LL)

static bool print_with_unit(u64 val, const char *unit, bool comma)
{
	if (!val)
		return comma;

	printf("%s%llu %s%s", comma ? ", " : "", val, unit, val > 1 ? "s" : "");
	return true;
}

static int do_uptime(int argc, char *argv[])
{
	u64 timestamp, weeks, days, hours, minutes;
	bool comma = false;
	int opt;

	timestamp = get_time_ns();

	while((opt = getopt(argc, argv, "n")) > 0) {
		switch(opt) {
		case 'n':
			printf("up %lluns\n", timestamp);
			return 0;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind != argc)
		return COMMAND_ERROR_USAGE;

	printf("up ");

	weeks = div64_u64_rem(timestamp, NSEC_PER_WEEK, &timestamp);
	days = div64_u64_rem(timestamp, NSEC_PER_DAY, &timestamp);
	hours = div64_u64_rem(timestamp, NSEC_PER_HOUR, &timestamp);
	minutes = div64_u64_rem(timestamp, NSEC_PER_MINUTE, &timestamp);

	comma = print_with_unit(weeks, "week", false);
	comma = print_with_unit(days, "day", comma);
	comma = print_with_unit(hours, "hour", comma);
	comma = print_with_unit(minutes, "minute", comma);

	if (!comma) {
		u64 seconds = div64_u64_rem(timestamp, NSEC_PER_SEC, &timestamp);
		print_with_unit(seconds, "second", false);
	}

	printf("\n");

	return 0;
}

BAREBOX_CMD_HELP_START(uptime)
BAREBOX_CMD_HELP_TEXT("This command formats the number of elapsed nanoseconds")
BAREBOX_CMD_HELP_TEXT("as measured with the current clocksource")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-n",     "output elapsed time in nanoseconds")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(uptime)
	.cmd		= do_uptime,
	BAREBOX_CMD_DESC("Tell how long barebox has been running")
	BAREBOX_CMD_OPTS("[-n]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_uptime_help)
BAREBOX_CMD_END
