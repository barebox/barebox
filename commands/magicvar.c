#include <common.h>
#include <command.h>
#include <getopt.h>
#include <environment.h>
#include <magicvar.h>

static int do_magicvar(int argc, char *argv[])
{
	struct magicvar *m;
	int opt;
	int verbose = 0;

	while ((opt = getopt(argc, argv, "v")) > 0) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	for (m = &__barebox_magicvar_start;
			m != &__barebox_magicvar_end;
			m++) {
		printf("%-32s %s\n", m->name, m->description);
		if (verbose) {
			const char *val = getenv(m->name);
			if (val && strlen(val))
				printf("  %s\n", val);
		}
	}

	return 0;
}


BAREBOX_CMD_HELP_START(magicvar)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v", "verbose (list all value if there is one)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(magicvar)
	.cmd		= do_magicvar,
	BAREBOX_CMD_DESC("list information about magic variables")
	BAREBOX_CMD_OPTS("[-v]")
	BAREBOX_CMD_HELP(cmd_magicvar_help)
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
BAREBOX_CMD_END
