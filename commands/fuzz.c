/* SPDX-License-Identifier: GPL-2.0 */

#define pr_fmt(fmt) "fuzz: " fmt

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <fuzz.h>
#include <libfile.h>
#include <fs.h>

struct fuzz_process {
	bool print;
	const char *match;
	unsigned nmatches;
	const struct fuzz_test *test;
};

static int process_fuzz_test(const struct fuzz_test *test,
			     void *_ctx)
{
	struct fuzz_process *ctx = _ctx;

	if (ctx->print) {
		printf("%s\n", test->name);
		ctx->nmatches++;
	}

	if (ctx->match && !strcmp(test->name, ctx->match)) {
		ctx->test = test;
		return true;
	}

	return false;
}

static const struct fuzz_test *get_fuzz_test(const char *match, bool print)
{
	struct fuzz_process ctx = {
		.match = match, .print = print
	};

	if (call_for_each_fuzz_test(process_fuzz_test, &ctx))
		return ctx.test;

	if (!ctx.nmatches) {
		if (match)
			printf("No fuzz tests matching '%s' found.\n", match);
		else
			printf("No fuzz tests registered.\n");
	}

	return NULL;
}

static int run_fuzz_test(const char *match, bool list, const u8 *buf, size_t len)
{
	const struct fuzz_test *test;

	test = get_fuzz_test(match, list);
	if (list)
		return 0;
	else if (!test)
		return COMMAND_ERROR;

	return fuzz_test_once(test, buf, len);
}

static int do_fuzz(int argc, char *argv[])
{
	const char *file = NULL;
	u64 max_size = FILESIZE_MAX;
	size_t size = 0;
	const u8 *buf = NULL;
	void *contents = NULL;
	bool list = false;
	int err = 0, opt;

	while((opt = getopt(argc, argv, "ls:f:")) > 0) {
		switch(opt) {
		case 'l':
			list = true;
			break;
		case 's':
			err = kstrtou64(optarg, 0, &max_size);
			if (err || max_size > SIZE_MAX) {
				printf("invalid max size\n");
				return COMMAND_ERROR;
			}
			break;
		case 'f':
			file = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if ((list && argc) || (!list && argc != 1) || (!file && !list))
		return COMMAND_ERROR_USAGE;

	if (file) {
		err = read_file_2(file, &size, &contents, max_size);
		if (err && err != -EFBIG) {
			printf("error reading file: %m\n");
			return COMMAND_ERROR;
		}

		buf = contents;
	}

	err = run_fuzz_test(argv[0], list, buf, size);

	free(contents);
	return err;
}

BAREBOX_CMD_HELP_START(fuzz)
BAREBOX_CMD_HELP_TEXT("Run barebox fuzz test on fixed input")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l",      "list registered fuzz tests")
BAREBOX_CMD_HELP_OPT ("-f FILE", "use FILE as fuzz input")
BAREBOX_CMD_HELP_OPT ("-s SIZE", "read only SIZE bytes from file")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(fuzz)
	.cmd		= do_fuzz,
	BAREBOX_CMD_DESC("Run fuzz test")
	BAREBOX_CMD_OPTS("[-ls] -f FILE [test]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_fuzz_help)
BAREBOX_CMD_END
