// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: (c) 2021 Ahmad Fatoum

#include <common.h>
#include <command.h>
#include <unistd.h>
#include <fcntl.h>
#include <complete.h>
#include <sys/stat.h>
#include <glob.h>
#include <getopt.h>
#include <magicvar.h>
#include <globalvar.h>

static int next_step;
BAREBOX_MAGICVAR(global.tutorial.step, "Next tutorial step.");

#define BUFSIZE 1024

static glob_t steps;

static int register_tutorial_vars(void)
{
	char *oldcwd;
	int ret = 0;

	oldcwd = pushd("/env/data/tutorial");
	if (!oldcwd)
		return 0;

	ret = glob("*", 0, NULL, &steps);
	if (ret)
		goto out;

	ret = globalvar_add_simple_enum("tutorial.step", &next_step,
					(const char **)steps.gl_pathv, steps.gl_pathc);

out:
	popd(oldcwd);
	return ret;

}
postenvironment_initcall(register_tutorial_vars);

static int print_tutorial_step(const char *step)
{
	bool highlight = false;
	int fd, ret, i;
	char *buf;

	fd = open(step, O_RDONLY);
	if (fd < 0) {
		printf("could not open /env/data/tutorial/%s\n", step);
		return -errno;
	}

	buf = malloc(BUFSIZE);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	printf("%s\n", step);

	while((ret = read(fd, buf, BUFSIZE)) > 0) {
		for(i = 0; i < ret; i++) {
			if (buf[i] != '`') {
				putchar(buf[i]);
				continue;
			}

			puts(highlight ? "\e[0m" : "\e[1;31m");
			highlight = !highlight;
		}
	}

	free(buf);
out:
	close(fd);

	return ret;
}

static int do_tutorial_next(int argc, char *argv[])
{
	int opt, i;
	char *step = NULL;
	char *oldcwd;
	ssize_t ret = 0;
	bool is_prev = *argv[0] == 'p';

	while ((opt = getopt(argc, argv, "rh")) > 0) {
		switch (opt) {
		case 'r':
			if (steps.gl_pathc) {
				globalvar_remove("tutorial.step");
				next_step = 0;
				globfree(&steps);
				steps.gl_pathc = 0;
			}
			ret = register_tutorial_vars();
			if (ret == 0 && steps.gl_pathc == 0)
				ret = -ENOENT;
			if (ret)
				printf("could not load /env/data/tutorial/\n");
			return ret;
		case 'h':
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind > argc + 1)
		return COMMAND_ERROR_USAGE;

	oldcwd = pushd("/env/data/tutorial");
	if (!oldcwd)
		return ret;

	if (is_prev)
		next_step = next_step > 0 ? next_step - 1 : 0;

	if (optind == argc) {
		step = steps.gl_pathv[next_step];
		ret = print_tutorial_step(step);
		if (ret == 0 && !is_prev)
			next_step = (next_step + 1) % steps.gl_pathc;
	} else {
		for (i = optind; i < argc; i++) {
			step = strdup(argv[i]);
			ret = print_tutorial_step(step);
			if (ret)
				break;
		}
	}

	popd(oldcwd);

	return ret;
}

BAREBOX_CMD_HELP_START(next)
BAREBOX_CMD_HELP_TEXT("Help navigate the barebox tutorial")
BAREBOX_CMD_HELP_TEXT("Without a parameter, the next or previous tutorial step is printed")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-h\t", "show usage text")
BAREBOX_CMD_HELP_OPT("-r\t", "rewind and reload tutorial")
BAREBOX_CMD_HELP_END

static __maybe_unused const char *const prev_alias[] = { "prev", NULL};

BAREBOX_CMD_START(next)
	.cmd		= do_tutorial_next,
	.aliases	= prev_alias,
	BAREBOX_CMD_DESC("navigate the barebox tutorial")
	BAREBOX_CMD_OPTS("[-hr] [STEP]")
	BAREBOX_CMD_HELP(cmd_next_help)
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(tutorial_complete)
BAREBOX_CMD_END
