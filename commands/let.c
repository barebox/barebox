// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2012 Jan Luebbe <j.luebbe@pengutronix.de>

/* Partially based on code from BusyBox. */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <init.h>
#include <malloc.h>
#include <xfuncs.h>
#include <math.h>

static void setvar(const char *name, const char *val)
{
	setenv(name, val); /* return value is always 0 */
}

static arith_t run_arith(const char *s)
{
	arith_state_t math_state;
	arith_t result;

	math_state.lookupvar = getenv;
	math_state.setvar    = setvar;
	math_state.endofname = arith_endofname;

	result = arith(&math_state, s);
	if (math_state.errmsg)
		printf("let: %s\n", math_state.errmsg);

	return result;
}

static int do_let(int argc, char *argv[])
{
	arith_t i;

	argv++;
	if (!*argv) {
		printf("let: expression expected\n");
		return COMMAND_ERROR_USAGE;
	}

	do {
		i = run_arith(*argv);
	} while (*++argv);

	return !i;
}

BAREBOX_CMD_HELP_START(let)
BAREBOX_CMD_HELP_TEXT ("Supported operations are in order of decreasing precedence:")
BAREBOX_CMD_HELP_TEXT ("	X++, X--")
BAREBOX_CMD_HELP_TEXT ("	++X, --X")
BAREBOX_CMD_HELP_TEXT ("	+X, -X")
BAREBOX_CMD_HELP_TEXT ("	!X, ~X")
BAREBOX_CMD_HELP_TEXT ("	X**Y")
BAREBOX_CMD_HELP_TEXT ("	X*Y, X/Y, X%Y")
BAREBOX_CMD_HELP_TEXT ("	X+Y, X-Y")
BAREBOX_CMD_HELP_TEXT ("	X<<Y, X>>Y")
BAREBOX_CMD_HELP_TEXT ("	X<Y, X<=Y, X>=Y, X>Y")
BAREBOX_CMD_HELP_TEXT ("	X==Y, X!=Y")
BAREBOX_CMD_HELP_TEXT ("	X&Y")
BAREBOX_CMD_HELP_TEXT ("	X^Y")
BAREBOX_CMD_HELP_TEXT ("	X|Y")
BAREBOX_CMD_HELP_TEXT ("	X&&Y")
BAREBOX_CMD_HELP_TEXT ("	X||Y")
BAREBOX_CMD_HELP_TEXT ("	X?Y:Z")
BAREBOX_CMD_HELP_TEXT ("	X*=Y, X/=Y, X%=Y")
BAREBOX_CMD_HELP_TEXT ("	X=Y, X&=Y, X|=Y, X^=Y, X+=Y, X-=Y, X<<=Y, X>>=Y")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(let)
	.cmd            = do_let,
	BAREBOX_CMD_DESC("evaluate arithmetic expressions")
	BAREBOX_CMD_OPTS("EXPR [EXPR ...]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_let_help)
BAREBOX_CMD_END
