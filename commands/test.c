// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/*
 * test.c - sh like test
 *
 * Originally based on barebox's do_test, but mostly reimplemented
 * for smaller binary size
 */
#include <common.h>
#include <command.h>
#include <fnmatch.h>
#include <environment.h>
#include <fs.h>
#include <linux/stat.h>

typedef enum {
	OPT_EQUAL,
	OPT_EQUAL_BASH,
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
	OPT_BLOCK,
	OPT_CHAR,
	OPT_SYMBOLIC_LINK,
	OPT_NONZERO_SIZE,
	OPT_VAR_EXISTS,
	OPT_MAX,
} test_opts;

static char *test_options[] = {
	[OPT_EQUAL]			= "=",
	[OPT_EQUAL_BASH]		= "==",
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
	[OPT_BLOCK]			= "-b",
	[OPT_CHAR]			= "-c",
	[OPT_SYMBOLIC_LINK]		= "-L",
	[OPT_NONZERO_SIZE]		= "-s",
	[OPT_VAR_EXISTS]		= "-v",
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

static int string_comp(const char *left_op, const char *right_op, bool bash_test)
{
	if (bash_test)
		return fnmatch(right_op, left_op, 0);

	return strcmp(left_op, right_op);
}

static int parse_number(const char *str, long *num, bool signedcmp)
{
	int ret;

	ret = signedcmp ? kstrtol(str, 0, num) : kstrtoul(str, 0, num);
	if (ret)
		printf("test: %s: integer expression expected\n", str);

	return ret;
}

#define __do_arith_cmp(x, op, y, signedcmp) \
	((signedcmp) ? (long)(x) op (long)(y) : (x) op (y))

static int arith_comp(const char *a_str, const char *b_str, int op)
{
	ulong a, b;
	bool signedcmp = a_str[0] == '-' || b_str[0] == '-';
	int ret;

	ret = parse_number(a_str, &a, signedcmp);
	if (ret)
		return ret;

	ret = parse_number(b_str, &b, signedcmp);
	if (ret)
		return ret;

	switch (op) {
	case OPT_ARITH_EQUAL:
		return __do_arith_cmp(a, ==, b, signedcmp);
	case OPT_ARITH_NOT_EQUAL:
		return __do_arith_cmp(a, !=, b, signedcmp);
	case OPT_ARITH_GREATER_EQUAL:
		return __do_arith_cmp(a, >=, b, signedcmp);
	case OPT_ARITH_GREATER_THAN:
		return __do_arith_cmp(a,  >, b, signedcmp);
	case OPT_ARITH_LESS_EQUAL:
		return __do_arith_cmp(a, <=, b, signedcmp);
	case OPT_ARITH_LESS_THAN:
		return __do_arith_cmp(a,  <, b, signedcmp);
	}

	return -EINVAL;
}

static int do_test(int argc, char *argv[])
{
	char **ap;
	int left, adv, expr, last_expr, neg, last_cmp, opt, zero;
	struct stat statbuf;
	bool bash_test = false;

	if (*argv[0] == '[') {
		argc--;
		if (!strncmp(argv[0], "[[", 2)) {
			if (strncmp(argv[argc], "]]", 2) != 0) {
				printf("[[: missing `]]'\n");
				return 1;
			}
			bash_test = true;
		} else {
			if (*argv[argc] != ']') {
				printf("[: missing `]'\n");
				return 1;
			}
		}
	}

	/* args? */
	if (argc < 2)
		return 1;

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
			if (left < 2)
				break;
			zero = 1;

			if (strlen(ap[1]))
				zero = 0;

			expr = (opt == OPT_ZERO) ? zero : !zero;
			break;

		case OPT_VAR_EXISTS:
			adv = 2;
			if (left < 2)
				break;
			expr = getenv(ap[1]) != NULL;
			break;

		case OPT_FILE:
		case OPT_DIRECTORY:
		case OPT_EXISTS:
		case OPT_BLOCK:
		case OPT_CHAR:
		case OPT_SYMBOLIC_LINK:
		case OPT_NONZERO_SIZE:
			adv = 2;
			if (left < 2)
				break;
			if (strlen(ap[1])) {
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
				if (opt == OPT_BLOCK && S_ISBLK(statbuf.st_mode)) {
					expr = 1;
					break;
				}
				if (opt == OPT_CHAR && S_ISCHR(statbuf.st_mode)) {
					expr = 1;
					break;
				}
				if (opt == OPT_NONZERO_SIZE && statbuf.st_size) {
					expr = 1;
					break;
				}
			}
			break;

		/* three argument options */
		default:
			adv = 3;
			if (left < 3)
				break;

			opt = parse_opt(ap[1]);
			switch (opt) {
			case OPT_EQUAL:
			case OPT_EQUAL_BASH:
				expr = string_comp(ap[0], ap[2], bash_test) == 0;
				break;
			case OPT_NOT_EQUAL:
				expr = string_comp(ap[0], ap[2], bash_test) != 0;
				break;
			case OPT_ARITH_EQUAL:
			case OPT_ARITH_NOT_EQUAL:
			case OPT_ARITH_LESS_THAN:
			case OPT_ARITH_LESS_EQUAL:
			case OPT_ARITH_GREATER_THAN:
			case OPT_ARITH_GREATER_EQUAL:
				expr = arith_comp(ap[0], ap[2], opt);
				if (expr < 0)
					return 1;
				break;
			default:
				expr = 1;
				goto out;
			}
		}

		if (left < adv) {
			printf("test: failed to parse arguments\n");
			return 1;
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

static const char * const test_aliases[] = { "[", "[[", NULL};

BAREBOX_CMD_HELP_START(test)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_TEXT("\t!, =, !=, -eq, -ne, -ge, -gt, -le, -lt, -o, -a, -z, -n, -d, -e,")
BAREBOX_CMD_HELP_TEXT("\t-s, -f, -L, -v; see 'man test' on your PC for more information.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(test)
	.aliases	= test_aliases,
	.cmd		= do_test,
	BAREBOX_CMD_DESC("minimal test command like in /bin/sh")
	BAREBOX_CMD_OPTS("[EXPR]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_test_help)
BAREBOX_CMD_END
