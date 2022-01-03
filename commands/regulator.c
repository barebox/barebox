// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* regulator command */

#include <common.h>
#include <command.h>
#include <regulator.h>
#include <getopt.h>

static int do_regulator(int argc, char *argv[])
{
	struct regulator *chosen;
	int opt, ret;

	while ((opt = getopt(argc, argv, "e:d:")) > 0) {
		switch (opt) {
		case 'e':
		case 'd':
			chosen = regulator_get_name(optarg);
			if (IS_ERR_OR_NULL(chosen)) {
				printf("regulator not found\n");
				return COMMAND_ERROR;
			}

			ret = opt == 'e' ? regulator_enable(chosen)
				         : regulator_disable(chosen);
			regulator_put(chosen);
			return ret;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	regulators_print();
	return 0;
}

BAREBOX_CMD_HELP_START(regulator)
	BAREBOX_CMD_HELP_TEXT("List and control regulators.")
	BAREBOX_CMD_HELP_TEXT("Without options, displays regulator info")
	BAREBOX_CMD_HELP_TEXT("Options:")
	BAREBOX_CMD_HELP_OPT("-e REGULATOR\t", "enable REGULATOR")
	BAREBOX_CMD_HELP_OPT("-d REGULATOR\t", "disable REGULATOR")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(regulator)
	.cmd		= do_regulator,
	BAREBOX_CMD_DESC("list and control regulators")
	BAREBOX_CMD_OPTS("[-ed] [REGULATOR]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_regulator_help)
BAREBOX_CMD_END
