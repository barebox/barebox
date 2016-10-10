/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <globalvar.h>
#include <command.h>
#include <common.h>
#include <getopt.h>
#include <malloc.h>
#include <boot.h>
#include <complete.h>

#include <linux/stat.h>

static int do_boot(int argc, char *argv[])
{
	char *freep = NULL;
	int opt, ret = 0, do_list = 0, do_menu = 0;
	int i, dryrun = 0, verbose = 0, timeout = -1;
	struct bootentries *entries;
	struct bootentry *entry;

	verbose = 0;
	dryrun = 0;
	timeout = -1;

	while ((opt = getopt(argc, argv, "vldmt:w:")) > 0) {
		switch (opt) {
		case 'v':
			verbose++;
			break;
		case 'l':
			do_list = 1;
			break;
		case 'd':
			dryrun = 1;
			break;
		case 'm':
			do_menu = 1;
			break;
		case 't':
			timeout = simple_strtoul(optarg, NULL, 0);
			break;
		case 'w':
			boot_set_watchdog_timeout(simple_strtoul(optarg, NULL, 0));
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	entries = bootentries_alloc();

	if (optind < argc) {
		for (i = optind; i < argc; i++) {
			ret = bootentry_create_from_name(entries, argv[i]);
			if (ret <= 0)
				printf("Nothing bootable found on '%s'\n", argv[i]);
	       }
	} else {
		const char *def;
		char *sep, *name;

		def = getenv("global.boot.default");
		if (!def)
			return 0;

		sep = freep = xstrdup(def);

		while ((name = strsep(&sep, " ")) != NULL) {
			ret = bootentry_create_from_name(entries, name);
			if (ret <= 0)
				printf("Nothing bootable found on '%s'\n", name);
		}

		free(freep);
	}

	if (list_empty(&entries->entries)) {
		printf("Nothing bootable found\n");
		return COMMAND_ERROR;
	}

	if (do_list) {
		bootsources_list(entries);
		goto out;
	}

	if (do_menu) {
		bootsources_menu(entries, timeout);
		goto out;
	}

	bootentries_for_each_entry(entries, entry) {
		ret = boot_entry(entry, verbose, dryrun);
		if (!ret)
			break;
	}

out:
	bootentries_free(entries);

	return ret;
}

BAREBOX_CMD_HELP_START(boot)
BAREBOX_CMD_HELP_TEXT("This is for booting based on scripts. Unlike the bootm command which")
BAREBOX_CMD_HELP_TEXT("can boot a single image this command offers the possibility to boot with")
BAREBOX_CMD_HELP_TEXT("scripts (by default placed under /env/boot/).")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("BOOTSRC can be:")
BAREBOX_CMD_HELP_TEXT("- a filename under /env/boot/")
BAREBOX_CMD_HELP_TEXT("- a full path to a boot script")
BAREBOX_CMD_HELP_TEXT("- a device name")
BAREBOX_CMD_HELP_TEXT("- a partition name under /dev/")
BAREBOX_CMD_HELP_TEXT("- a full path to a directory which")
BAREBOX_CMD_HELP_TEXT("   - contains boot scripts, or")
BAREBOX_CMD_HELP_TEXT("   - contains a loader/entries/ directory containing bootspec entries")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Multiple bootsources may be given which are probed in order until")
BAREBOX_CMD_HELP_TEXT("one succeeds.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v","Increase verbosity")
BAREBOX_CMD_HELP_OPT ("-d","Dryrun. See what happens but do no actually boot")
BAREBOX_CMD_HELP_OPT ("-l","List available boot sources")
BAREBOX_CMD_HELP_OPT ("-m","Show a menu with boot options")
BAREBOX_CMD_HELP_OPT ("-w SECS","Start watchdog with timeout SECS before booting")
BAREBOX_CMD_HELP_OPT ("-t SECS","specify timeout in SECS")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(boot)
	.cmd	= do_boot,
	BAREBOX_CMD_DESC("boot from script, device, ...")
	BAREBOX_CMD_OPTS("[-vdlmwt] [BOOTSRC...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_boot_help)
BAREBOX_CMD_END
