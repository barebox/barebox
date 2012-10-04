/*
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
 *
 * Partially based on code from BusyBox.
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
 *
 */

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
BAREBOX_CMD_HELP_USAGE("let expr [expr ...]\n")
BAREBOX_CMD_HELP_SHORT("evaluate arithmetic expressions\n")
BAREBOX_CMD_HELP_TEXT ("supported operations are in order of decreasing precedence:\n")
BAREBOX_CMD_HELP_TEXT ("	X++, X--\n")
BAREBOX_CMD_HELP_TEXT ("	++X, --X\n")
BAREBOX_CMD_HELP_TEXT ("	+X, -X\n")
BAREBOX_CMD_HELP_TEXT ("	!X, ~X\n")
BAREBOX_CMD_HELP_TEXT ("	X**Y\n")
BAREBOX_CMD_HELP_TEXT ("	X*Y, X/Y, X%Y\n")
BAREBOX_CMD_HELP_TEXT ("	X+Y, X-Y\n")
BAREBOX_CMD_HELP_TEXT ("	X<<Y, X>>Y\n")
BAREBOX_CMD_HELP_TEXT ("	X<Y, X<=Y, X>=Y, X>Y\n")
BAREBOX_CMD_HELP_TEXT ("	X==Y, X!=Y\n")
BAREBOX_CMD_HELP_TEXT ("	X&Y\n")
BAREBOX_CMD_HELP_TEXT ("	X^Y\n")
BAREBOX_CMD_HELP_TEXT ("	X|Y\n")
BAREBOX_CMD_HELP_TEXT ("	X&&Y\n")
BAREBOX_CMD_HELP_TEXT ("	X||Y\n")
BAREBOX_CMD_HELP_TEXT ("	X?Y:Z\n")
BAREBOX_CMD_HELP_TEXT ("	X*=Y, X/=Y, X%=Y\n")
BAREBOX_CMD_HELP_TEXT ("	X=Y, X&=Y, X|=Y, X^=Y, X+=Y, X-=Y, X<<=Y, X>>=Y\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(let)
	.cmd            = do_let,
	.usage          = "evaluate arithmetic expressions",
	BAREBOX_CMD_HELP(cmd_let_help)
BAREBOX_CMD_END
