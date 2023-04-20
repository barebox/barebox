// SPDX-License-Identifier: GPL-2.0-only

/*
 * of_fixup.c - List and remove OF fixups
 */

#include <common.h>
#include <kallsyms.h>
#include <of.h>
#include <command.h>
#include <malloc.h>
#include <complete.h>
#include <getopt.h>
#include <string.h>

static int do_of_fixup(int argc, char *argv[])
{
	struct of_fixup *of_fixup;
	int opt, enable = -1;
	bool did_fixup = false;

	while ((opt = getopt(argc, argv, "ed")) > 0) {
		switch (opt) {
		case 'e':
			enable = 1;
			break;
		case 'd':
			enable = 0;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if ((enable < 0 && argc > 0) || (enable >= 0 && argc == 0))
		return COMMAND_ERROR_USAGE;

	list_for_each_entry(of_fixup, &of_fixup_list, list) {
		int i;
		ulong addr = (ulong)of_fixup->fixup;
		char sym[KSYM_SYMBOL_LEN];
		const char *name;

		name = kallsyms_lookup(addr, NULL, NULL, NULL, sym);
		if (!name) {
			sprintf(sym, "<0x%lx>", addr);
			name = sym;
		}

		if (enable == -1) {
			printf("%s(0x%p)%s\n", name, of_fixup->context,
			       of_fixup->disabled ? " [DISABLED]" : "");
			continue;
		}

		for (i = 0; i < argc; i++) {
			if (strcmp(name, argv[i]) != 0)
				continue;

			of_fixup->disabled = !enable;
			did_fixup = true;
		}
	}

	if (argc && !did_fixup) {
		printf("none of the specified fixups found\n");
		return -EINVAL;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(of_fixup)
BAREBOX_CMD_HELP_TEXT("Disable or re-enable an already registered fixup for the device tree.")
BAREBOX_CMD_HELP_TEXT("Call without arguments to list all fixups")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-d",  "disable fixup")
BAREBOX_CMD_HELP_OPT("-e",  "re-enable fixup")
BAREBOX_CMD_HELP_OPT("fixups",  "List of fixups to enable or disable")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_fixup)
	.cmd	= do_of_fixup,
	BAREBOX_CMD_DESC("list and enable/disable fixups")
	BAREBOX_CMD_OPTS("[-de] [fixups...]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_of_fixup_help)
BAREBOX_CMD_END
