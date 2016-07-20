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

#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <watchdog.h>
#include <command.h>
#include <readkey.h>
#include <common.h>
#include <getopt.h>
#include <blspec.h>
#include <libgen.h>
#include <malloc.h>
#include <clock.h>
#include <bootm.h>
#include <glob.h>
#include <init.h>
#include <menu.h>
#include <fs.h>
#include <complete.h>

#include <linux/stat.h>

static int verbose;
static int dryrun;
static int timeout;

int bootentries_add_entry(struct bootentries *entries, struct bootentry *entry)
{
	list_add_tail(&entry->list, &entries->entries);

	return 0;
}

static struct bootentries *bootentries_alloc(void)
{
	struct bootentries *bootentries;

	bootentries = xzalloc(sizeof(*bootentries));
	INIT_LIST_HEAD(&bootentries->entries);

	if (IS_ENABLED(CONFIG_MENU)) {
		bootentries->menu = menu_alloc();
		bootentries->menu->display = basprintf("boot");
	}

	return bootentries;
}

static void bootentries_free(struct bootentries *bootentries)
{
	struct bootentry *be, *tmp;

	list_for_each_entry_safe(be, tmp, &bootentries->entries, list) {
		list_del(&be->list);
		free(be->title);
		free(be->description);
		free(be->me.display);
		be->release(be);
	}

	if (bootentries->menu)
		free(bootentries->menu->display);
	free(bootentries->menu);
	free(bootentries);
}

struct bootentry_script {
	struct bootentry entry;
	char *scriptpath;
};

/*
 * Start a single boot script. 'path' is a full path to a boot script.
 */
static int bootscript_boot(struct bootentry *entry, int verbose, int dryrun)
{
	struct bootentry_script *bs = container_of(entry, struct bootentry_script, entry);
	int ret;

	struct bootm_data data = {};

	if (dryrun) {
		printf("Would run %s\n", bs->scriptpath);
		return 0;
	}

	globalvar_set_match("linux.bootargs.dyn.", "");

	ret = run_command(bs->scriptpath);
	if (ret) {
		printf("Running %s failed\n", bs->scriptpath);
		goto out;
	}

	bootm_data_init_defaults(&data);

	if (verbose)
		data.verbose = verbose;
	if (dryrun)
		data.dryrun = dryrun;

	ret = bootm_boot(&data);
	if (ret)
		pr_err("Booting %s failed: %s\n", basename(bs->scriptpath), strerror(-ret));
out:
	return ret;
}

static unsigned int boot_watchdog_timeout;

static int init_boot_watchdog_timeout(void)
{
	return globalvar_add_simple_int("boot.watchdog_timeout",
			&boot_watchdog_timeout, "%u");
}
late_initcall(init_boot_watchdog_timeout);

BAREBOX_MAGICVAR_NAMED(global_watchdog_timeout, global.boot.watchdog_timeout,
		"Watchdog enable timeout in seconds before booting");

static int boot_entry(struct bootentry *be)
{
	int ret;

	printf("booting %s\n", be->title);

	if (IS_ENABLED(CONFIG_WATCHDOG) && boot_watchdog_timeout) {
		ret = watchdog_set_timeout(boot_watchdog_timeout);
		if (ret)
			pr_warn("Failed to enable watchdog: %s\n", strerror(-ret));
	}

	ret = be->boot(be, verbose, dryrun);

	if (ret)
		printf("booting %s failed: %s\n", be->title, strerror(-ret));

	return ret;
}

static void bootsource_action(struct menu *m, struct menu_entry *me)
{
	struct bootentry *be = container_of(me, struct bootentry, me);
	int ret;

	ret = boot_entry(be);
	if (ret)
		printf("Booting failed with: %s\n", strerror(-ret));

	printf("Press any key to continue\n");

	read_key();
}

static void bootscript_entry_release(struct bootentry *entry)
{
	struct bootentry_script *bs = container_of(entry, struct bootentry_script, entry);

	free(bs->scriptpath);
	free(bs->entry.me.display);
	free(bs);
}

/*
 * bootscript_create_entry - create a boot entry from a script name
 */
static int bootscript_create_entry(struct bootentries *bootentries, const char *name)
{
	struct bootentry_script *bs;
	enum filetype type;

	type = file_name_detect_type(name);
	if (type != filetype_sh)
		return -EINVAL;

	bs = xzalloc(sizeof(*bs));
	bs->entry.me.type = MENU_ENTRY_NORMAL;
	bs->entry.release = bootscript_entry_release;
	bs->entry.boot = bootscript_boot;
	bs->scriptpath = xstrdup(name);
	bs->entry.title = xstrdup(basename(bs->scriptpath));
	bs->entry.description = basprintf("script: %s", name);
	bootentries_add_entry(bootentries, &bs->entry);

	return 0;
}

/*
 * bootscript_scan_path - create boot entries from a path
 *
 * path can either be a full path to a bootscript or a full path to a diretory
 * containing bootscripts.
 */
static int bootscript_scan_path(struct bootentries *bootentries, const char *path)
{
	struct stat s;
	char *files;
	int ret, i;
	int found = 0;
	glob_t globb;

	ret = stat(path, &s);
	if (ret)
		return ret;

	if (!S_ISDIR(s.st_mode)) {
		ret = bootscript_create_entry(bootentries, path);
		if (ret)
			return ret;
		return 1;
	}

	files = basprintf("%s/*", path);

	glob(files, 0, NULL, &globb);

	for (i = 0; i < globb.gl_pathc; i++) {
		char *bootscript_path = globb.gl_pathv[i];

		if (*basename(bootscript_path) == '.')
			continue;

		bootscript_create_entry(bootentries, bootscript_path);
		found++;
	}

	globfree(&globb);
	free(files);

	ret = found;

	return ret;
}

/*
 * bootentry_parse_one - create boot entries from a name
 *
 * name can be:
 * - a name of a boot script under /env/boot
 * - a full path of a boot script
 * - a device name
 * - a cdev name
 * - a full path of a directory containing bootloader spec entries
 * - a full path of a directory containing bootscripts
 *
 * Returns the number of entries found or a negative error code.
 */
static int bootentry_parse_one(struct bootentries *bootentries, const char *name)
{
	int found = 0, ret;

	if (IS_ENABLED(CONFIG_BLSPEC)) {
		ret = blspec_scan_devicename(bootentries, name);
		if (ret > 0)
			found += ret;

		if (*name == '/') {
			ret = blspec_scan_directory(bootentries, name);
			if (ret > 0)
				found += ret;
		}
	}

	if (!found) {
		char *path;

		if (*name != '/')
			path = basprintf("/env/boot/%s", name);
		else
			path = xstrdup(name);

		ret = bootscript_scan_path(bootentries, path);
		if (ret > 0)
			found += ret;

		free(path);
	}

	return found;
}

/*
 * bootsources_menu - show a menu from an array of names
 */
static void bootsources_menu(struct bootentries *bootentries)
{
	struct bootentry *entry;
	struct menu_entry *back_entry;

	if (!IS_ENABLED(CONFIG_MENU)) {
		printf("no menu support available\n");
		return;
	}

	bootentries_for_each_entry(bootentries, entry) {
		if (!entry->me.display)
			entry->me.display = xstrdup(entry->title);
		entry->me.action = bootsource_action;
		menu_add_entry(bootentries->menu, &entry->me);
	}

	back_entry = xzalloc(sizeof(*back_entry));
	back_entry->display = "back";
	back_entry->type = MENU_ENTRY_NORMAL;
	back_entry->non_re_ent = 1;
	menu_add_entry(bootentries->menu, back_entry);

	if (timeout >= 0)
		bootentries->menu->auto_select = timeout;

	menu_show(bootentries->menu);

	free(back_entry);
}

/*
 * bootsources_list - list boot entries from an array of names
 */
static void bootsources_list(struct bootentries *bootentries)
{
	struct bootentry *entry;

	printf("%-20s\n", "title");
	printf("%-20s\n", "------");

	bootentries_for_each_entry(bootentries, entry)
		printf("%-20s %s\n", entry->title, entry->description);
}

static int do_boot(int argc, char *argv[])
{
	char *freep = NULL;
	int opt, ret = 0, do_list = 0, do_menu = 0;
	int i;
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
			boot_watchdog_timeout = simple_strtoul(optarg, NULL, 0);
			break;
		}
	}

	entries = bootentries_alloc();

	if (optind < argc) {
		for (i = optind; i < argc; i++)
			bootentry_parse_one(entries, argv[i]);
	} else {
		const char *def;
		char *sep, *name;

		def = getenv("global.boot.default");
		if (!def)
			return 0;

		sep = freep = xstrdup(def);

		while ((name = strsep(&sep, " ")) != NULL)
			bootentry_parse_one(entries, name);

		free(freep);
	}

	if (do_list) {
		bootsources_list(entries);
		goto out;
	}

	if (do_menu) {
		bootsources_menu(entries);
		goto out;
	}

	bootentries_for_each_entry(entries, entry) {
		ret = boot_entry(entry);
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
	BAREBOX_CMD_OPTS("[-vdlmt] [BOOTSRC...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_boot_help)
BAREBOX_CMD_END

BAREBOX_MAGICVAR_NAMED(global_boot_default, global.boot.default, "default boot order");
