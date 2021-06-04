/* SPDX-License-Identifier: GPL-2.0 */

#define pr_fmt(fmt) "selftest: " fmt

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <bselftest.h>
#include <complete.h>

static int run_selftest(const char *match, bool list)
{
	struct selftest *test;
	int matches = 0;
	int err = 0;

	list_for_each_entry(test, &selftests, list) {
		if (list) {
			printf("%s\n", test->name);
			matches++;
			continue;
		}

		if (match && strcmp(test->name, match))
			continue;

		err |= test->func();
		matches++;
	}

	if (!matches) {
		if (match) {
			printf("No tests matching '%s' found.\n", match);
			return -EINVAL;
		}

		printf("No tests found.\n");
	}

	return err;
}

static int do_selftest(int argc, char *argv[])
{
	bool list = false;
	int i, err = 0;
	int opt;

	while((opt = getopt(argc, argv, "l")) > 0) {
		switch(opt) {
		case 'l':
			list = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc) {
		err = run_selftest(NULL, list);
	} else {
		for (i = optind; i < argc; i++) {
			err = run_selftest(argv[i], list);
			if (err)
				goto out;
		}
	}

out:
	return err ? COMMAND_ERROR : COMMAND_SUCCESS;
}

BAREBOX_CMD_HELP_START(selftest)
BAREBOX_CMD_HELP_TEXT("Run enabled barebox self-tests")
BAREBOX_CMD_HELP_TEXT("If run without arguments, all tests are run")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l",     "list available tests")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(selftest)
	.cmd		= do_selftest,
	BAREBOX_CMD_DESC("Run selftests")
	BAREBOX_CMD_OPTS("[-l] [tests..]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_selftest_help)
BAREBOX_CMD_END
