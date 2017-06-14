/*
 * test.c - sh like test
 *
 * Originally based on bareboxs do_test, but mostly reimplemented
 * for smaller binary size
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <command.h>
#include <fs.h>
#include <linux/stat.h>

typedef enum {
	OPT_EQUAL,
	OPT_NOT_EQUAL,
	OPT_ARITH_EQUAL,
	OPT_ARITH_NOT_EQUAL,
	OPT_ARITH_GREATER_EQUAL,
	OPT_ARITH_GREATER_THAN,
	OPT_ARITH_LESS_EQUAL,
	OPT_ARITH_LESS_THAN,
	OPT_OR,
	OPT_AND,
	OPT_ZERO,
	OPT_NONZERO,
	OPT_DIRECTORY,
	OPT_FILE,
	OPT_EXISTS,
	OPT_SYMBOLIC_LINK,
	OPT_MAX,
} test_opts;

static char *test_options[] = {
	[OPT_EQUAL]			= "=",
	[OPT_NOT_EQUAL]			= "!=",
	[OPT_ARITH_EQUAL]		= "-eq",
	[OPT_ARITH_NOT_EQUAL]		= "-ne",
	[OPT_ARITH_GREATER_EQUAL]	= "-ge",
	[OPT_ARITH_GREATER_THAN]	= "-gt",
	[OPT_ARITH_LESS_EQUAL]		= "-le",
	[OPT_ARITH_LESS_THAN]		= "-lt",
	[OPT_OR]			= "-o",
	[OPT_AND]			= "-a",
	[OPT_ZERO]			= "-z",
	[OPT_NONZERO]			= "-n",
	[OPT_FILE]			= "-f",
	[OPT_DIRECTORY]			= "-d",
	[OPT_EXISTS]			= "-e",
	[OPT_SYMBOLIC_LINK]		= "-L",
};

static int parse_opt(const char *opt)
{
	char **opts = test_options;
	int i;

	for (i = 0; i < OPT_MAX; i++) {
		if (!strcmp(opts[i], opt))
			return i;
	}
	return -1;
}

static int do_test(int argc, char *argv[])
{
	char **ap;
	int left, adv, expr, last_expr, neg, last_cmp, opt, zero;
	ulong a, b;
	struct stat statbuf;

	if (*argv[0] == '[') {
		if (*argv[argc - 1] != ']') {
			printf("[: missing `]'\n");
			return 1;
		}
		argc--;
	}

	/* args? */
	if (argc < 2)
		return 1;

	last_expr = 0;
	left = argc - 1;
	ap = argv + 1;

	if (strcmp(ap[0], "!") == 0) {
		neg = 1;
		ap++;
		left--;
	} else
		neg = 0;

	expr = -1;
	last_cmp = -1;
	last_expr = -1;
	adv = 0;
	while (left - adv > 0) {
		ap += adv; left -= adv;

		adv = 1;
		opt = parse_opt(ap[0]);
		switch (opt) {
		/* one argument options */
		case OPT_OR:
			last_expr = expr;
			last_cmp = 0;
			continue;
		case OPT_AND:
			last_expr = expr;
			last_cmp = 1;
			continue;

		/* two argument options */
		case OPT_ZERO:
		case OPT_NONZERO:
			adv = 2;
			zero = 1;
			if (ap[1] && *ap[1] != ']' && strlen(ap[1]))
				zero = 0;

			expr = (opt == OPT_ZERO) ? zero : !zero;
			break;

		case OPT_FILE:
		case OPT_DIRECTORY:
		case OPT_EXISTS:
		case OPT_SYMBOLIC_LINK:
			adv = 2;
			if (ap[1] && *ap[1] != ']' && strlen(ap[1])) {
				expr = (opt == OPT_SYMBOLIC_LINK ? lstat : stat)(ap[1], &statbuf);
				if (expr < 0) {
					expr = 0;
					break;
				}
				expr = 0;
				if (opt == OPT_EXISTS) {
					expr = 1;
					break;
				}
				if (opt == OPT_FILE && S_ISREG(statbuf.st_mode)) {
					expr = 1;
					break;
				}
				if (opt == OPT_DIRECTORY && S_ISDIR(statbuf.st_mode)) {
					expr = 1;
					break;
				}
				if (opt == OPT_SYMBOLIC_LINK && S_ISLNK(statbuf.st_mode)) {
					expr = 1;
					break;
				}
			}
			break;

		/* three argument options */
		default:
			adv = 3;
			if (left < 3) {
				expr = 1;
				break;
			}

			a = simple_strtol(ap[0], NULL, 0);
			b = simple_strtol(ap[2], NULL, 0);
			switch (parse_opt(ap[1])) {
			case OPT_EQUAL:
				expr = strcmp(ap[0], ap[2]) == 0;
				break;
			case OPT_NOT_EQUAL:
				expr = strcmp(ap[0], ap[2]) != 0;
				break;
			case OPT_ARITH_EQUAL:
				expr = a == b;
				break;
			case OPT_ARITH_NOT_EQUAL:
				expr = a != b;
				break;
			case OPT_ARITH_LESS_THAN:
				expr = a < b;
				break;
			case OPT_ARITH_LESS_EQUAL:
				expr = a <= b;
				break;
			case OPT_ARITH_GREATER_THAN:
				expr = a > b;
				break;
			case OPT_ARITH_GREATER_EQUAL:
				expr = a >= b;
				break;
			default:
				expr = 1;
				goto out;
			}
		}

		if (last_cmp == 0)
			expr = last_expr || expr;
		else if (last_cmp == 1)
			expr = last_expr && expr;
		last_cmp = -1;
	}
out:
	if (neg)
		expr = !expr;

	expr = !expr;


	return expr;
}

static const char * const test_aliases[] = { "[", NULL};

BAREBOX_CMD_HELP_START(test)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_TEXT("\t!, =, !=, -eq, -ne, -ge, -gt, -le, -lt, -o, -a, -z, -n, -d, -e,")
BAREBOX_CMD_HELP_TEXT("\t-f, -L; see 'man test' on your PC for more information.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(test)
	.aliases	= test_aliases,
	.cmd		= do_test,
	BAREBOX_CMD_DESC("minimal test command like in /bin/sh")
	BAREBOX_CMD_OPTS("[EXPR]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_test_help)
BAREBOX_CMD_END
