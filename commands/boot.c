// SPDX-License-Identifier: GPL-2.0-or-later

#include <globalvar.h>
#include <command.h>
#include <common.h>
#include <getopt.h>
#include <malloc.h>
#include <boot.h>
#include <bootm.h>
#include <complete.h>

#include <linux/stat.h>

static char *next_argv(void *context)
{
	char ***argv = context;
	char *next = **argv;
	(*argv)++;
	return next;
}

static char *next_word(void *context)
{
	return strsep(context, " ");
}

static int boot_add_override(struct bootm_overrides *overrides, char *var)
{
	const char *val;

	if (!IS_ENABLED(CONFIG_BOOT_OVERRIDE))
		return -ENOSYS;

	var += str_has_prefix(var, "global.");

	val = parse_assignment(var);
	if (!val) {
		val = globalvar_get(var);
		if (isempty(val))
			val = NULL;
	}

	if (!strcmp(var, "bootm.image")) {
		if (isempty(val))
			return -EINVAL;
		return -ENOSYS;
	} else if (!strcmp(var, "bootm.oftree")) {
		overrides->oftree_file = val;
	} else if (!strcmp(var, "bootm.initrd")) {
		overrides->initrd_file = val;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int do_boot(int argc, char *argv[])
{
	char *freep = NULL;
	int opt, ret = 0, do_list = 0, do_menu = 0;
	int dryrun = 0, verbose = 0, timeout = -1;
	unsigned default_menu_entry = 0;
	struct bootentries *entries;
	struct bootentry *entry;
	struct bootm_overrides overrides = {};
	void *handle;
	const char *name;
	char *(*next)(void *);

	while ((opt = getopt(argc, argv, "vldmM:t:w:o:")) > 0) {
		switch (opt) {
		case 'v':
			verbose++;
			break;
		case 'l':
			do_list = 1;
			break;
		case 'd':
			dryrun++;
			break;
		case 'M':
			/* To simplify scripting, an empty string is treated as 1 */
			if (*optarg == '\0') {
				default_menu_entry = 1;
			} else {
				ret = kstrtouint(optarg, 0, &default_menu_entry);
				if (ret)
					return ret;
			}
			fallthrough;
		case 'm':
			do_menu = 1;
			break;
		case 't':
			timeout = simple_strtoul(optarg, NULL, 0);
			break;
		case 'w':
			boot_set_watchdog_timeout(simple_strtoul(optarg, NULL, 0));
			break;
		case 'o':
			ret = boot_add_override(&overrides, optarg);
			if (ret)
				return ret;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind < argc) {
		handle = &argv[optind];
		next = next_argv;
	} else {
		const char *def;

		def = getenv("global.boot.default");
		if (!def)
			return 0;

		handle = freep = xstrdup(def);
		next = next_word;
	}

	entries = bootentries_alloc();

	while ((name = next(&handle)) != NULL) {
		if (!*name)
			continue;
		ret = bootentry_create_from_name(entries, name);
		if (ret <= 0)
			printf("Nothing bootable found on '%s'\n", name);

		if (do_list || do_menu)
			continue;

		bootentries_for_each_entry(entries, entry) {
			bootm_merge_overrides(&entry->overrides, &overrides);

			ret = boot_entry(entry, verbose, dryrun);
			if (!ret)
				goto out;
		}

		bootentries_free(entries);
		entries = bootentries_alloc();
	}

	if (list_empty(&entries->entries)) {
		printf("Nothing bootable found\n");
		ret = COMMAND_ERROR;
		goto out;
	}

	if (do_list)
		bootsources_list(entries);
	else if (do_menu)
		bootsources_menu(entries, default_menu_entry, timeout);

	ret = 0;
out:
	bootentries_free(entries);
	free(freep);

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
BAREBOX_CMD_HELP_TEXT("- a full path to a bootspec entry")
BAREBOX_CMD_HELP_TEXT("- a device name")
BAREBOX_CMD_HELP_TEXT("- a partition name under /dev/")
BAREBOX_CMD_HELP_TEXT("- a full path to a directory which")
BAREBOX_CMD_HELP_TEXT("   - contains boot scripts, or")
BAREBOX_CMD_HELP_TEXT("   - contains a loader/entries/ directory containing bootspec entries")
#ifdef CONFIG_BOOTCHOOSER
BAREBOX_CMD_HELP_TEXT("- \"bootchooser\": boot with barebox bootchooser")
#endif
#ifdef CONFIG_BOOT_DEFAULTS
BAREBOX_CMD_HELP_TEXT("- \"bootsource\": boot from the device barebox has been started from")
#endif
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Multiple bootsources may be given which are probed in order until")
BAREBOX_CMD_HELP_TEXT("one succeeds.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v","Increase verbosity")
BAREBOX_CMD_HELP_OPT ("-d","Dryrun. See what happens but do no actually boot (pass twice to run scripts)")
BAREBOX_CMD_HELP_OPT ("-l","List available boot sources")
BAREBOX_CMD_HELP_OPT ("-m","Show a menu with boot options")
BAREBOX_CMD_HELP_OPT ("-M INDEX","Show a menu with boot options with entry INDEX preselected")
BAREBOX_CMD_HELP_OPT ("-w SECS","Start watchdog with timeout SECS before booting")
#ifdef CONFIG_BOOT_OVERRIDE
BAREBOX_CMD_HELP_OPT ("-o VAR[=VAL]","override VAR (bootm.{oftree,initrd}) with VAL")
BAREBOX_CMD_HELP_OPT ("            ","if VAL is not specified, the value of VAR is taken")
#endif
BAREBOX_CMD_HELP_OPT ("-t SECS","specify timeout in SECS")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(boot)
	.cmd	= do_boot,
	BAREBOX_CMD_DESC("boot from script, device, ...")
	BAREBOX_CMD_OPTS("[-vdlmMwt] [BOOTSRC...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_boot_help)
BAREBOX_CMD_END
