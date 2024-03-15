// SPDX-License-Identifier: GPL-2.0-only

/*
 * kallsyms.c - translate address to symbols
 */

#include <common.h>
#include <kallsyms.h>
#include <command.h>
#include <malloc.h>
#include <complete.h>
#include <getopt.h>
#include <string.h>

static int do_kallsyms(int argc, char *argv[])
{
	unsigned long addr;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	if (kstrtoul(argv[1], 16, &addr) == 0) {
		char sym[KSYM_SYMBOL_LEN];

		sprint_symbol(sym, addr);

		printf("%s\n", sym);
		return 0;
	}

	if ((addr = kallsyms_lookup_name(argv[1]))) {
		printf("0x%08lx\n", addr);
		return 0;
	}

	return COMMAND_ERROR;
}

BAREBOX_CMD_HELP_START(kallsyms)
BAREBOX_CMD_HELP_TEXT("Lookup address or symbol using kallsyms table")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(kallsyms)
	.cmd	= do_kallsyms,
	BAREBOX_CMD_DESC("query kallsyms table")
	BAREBOX_CMD_OPTS("[SYMBOL | ADDRESS]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_kallsyms_help)
BAREBOX_CMD_END
