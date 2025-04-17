// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2017 Oleksij Rempel <kernel@pengutronix.de>

#include <common.h>
#include <command.h>
#include <stdlib.h>
#include <linux/ctype.h>

static int do_seed(int argc, char *argv[])
{
	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	if (isdigit(*argv[1])) {
		srand_xor(simple_strtoull(argv[1], NULL, 0));
		return 0;
	}

	printf("numerical parameter expected\n");
	return 1;
}

BAREBOX_CMD_HELP_START(seed)
BAREBOX_CMD_HELP_TEXT("Seed the pseudo random number generator")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(seed)
	.cmd = do_seed,
	BAREBOX_CMD_DESC("seed the PRNG")
	BAREBOX_CMD_OPTS("VALUE")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_seed_help)
BAREBOX_CMD_END
