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
#include <command.h>
#include <readkey.h>
#include <common.h>
#include <getopt.h>
#include <blspec.h>
#include <libgen.h>
#include <malloc.h>
#include <clock.h>
#include <boot.h>
#include <glob.h>
#include <menu.h>
#include <fs.h>
#include <complete.h>

#include <linux/stat.h>

static int verbose;
static int dryrun;
static int timeout;

/*
 * Start a single boot script. 'path' is a full path to a boot script.
 */
static int boot_script(char *path)
{
	int ret;
	struct bootm_data data = {
		.os_address = UIMAGE_SOME_ADDRESS,
		.initrd_address = UIMAGE_SOME_ADDRESS,
	};

	globalvar_set_match("linux.bootargs.dyn.", "");
	globalvar_set_match("bootm.", "");

	ret = run_command(path);
	if (ret) {
		printf("Running %s failed\n", path);
		goto out;
	}

	data.initrd_address = UIMAGE_INVALID_ADDRESS;
	data.os_address = UIMAGE_SOME_ADDRESS;
	data.oftree_file = getenv_nonempty("global.bootm.oftree");
	data.os_file = getenv_nonempty("global.bootm.image");
	getenv_ul("global.bootm.image.loadaddr", &data.os_address);
	getenv_ul("global.bootm.initrd.loadaddr", &data.initrd_address);
	data.initrd_file = getenv_nonempty("global.bootm.initrd");
	data.verbose = verbose;
	data.dryrun = dryrun;

	ret = bootm_boot(&data);
	if (ret)
		pr_err("Booting %s failed: %s\n", basename(path), strerror(-ret));
out:
	return ret;
}

static int boot_entry(struct blspec_entry *be)
{
	int ret;

	if (be->scriptpath) {
		ret = boot_script(be->scriptpath);
	} else {
		if (IS_ENABLED(CONFIG_BLSPEC))
			ret = blspec_boot(be, verbose, dryrun);
		else
			ret = -ENOSYS;
	}

	return ret;
}

static void bootsource_action(struct menu *m, struct menu_entry *me)
{
	struct blspec_entry *be = container_of(me, struct blspec_entry, me);
	int ret;

	ret = boot_entry(be);
	if (ret)
		printf("Booting failed with: %s\n", strerror(-ret));

	printf("Press any key to continue\n");

	read_key();
}

/*
 * bootscript_create_entry - create a boot entry from a script name
 */
static int bootscript_create_entry(struct blspec *blspec, const char *name)
{
	struct blspec_entry *be;
	enum filetype type;

	type = file_name_detect_type(name);
	if (type != filetype_sh)
		return -EINVAL;

	be = blspec_entry_alloc(blspec);
	be->me.type = MENU_ENTRY_NORMAL;
	be->scriptpath = xstrdup(name);
	be->me.display = xstrdup(basename(be->scriptpath));

	return 0;
}

/*
 * bootscript_scan_path - create boot entries from a path
 *
 * path can either be a full path to a bootscript or a full path to a diretory
 * containing bootscripts.
 */
static int bootscript_scan_path(struct blspec *blspec, const char *path)
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
		ret = bootscript_create_entry(blspec, path);
		if (ret)
			return ret;
		return 1;
	}

	files = asprintf("%s/*", path);

	glob(files, 0, NULL, &globb);

	for (i = 0; i < globb.gl_pathc; i++) {
		char *bootscript_path = globb.gl_pathv[i];;

		if (*basename(bootscript_path) == '.')
			continue;

		bootscript_create_entry(blspec, bootscript_path);
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
static int bootentry_parse_one(struct blspec *blspec, const char *name)
{
	int found = 0, ret;

	if (IS_ENABLED(CONFIG_BLSPEC)) {
		ret = blspec_scan_devicename(blspec, name);
		if (ret > 0)
			found += ret;
		ret = blspec_scan_directory(blspec, name);
		if (ret > 0)
			found += ret;
	}

	if (!found) {
		char *path;

		if (*name != '/')
			path = asprintf("/env/boot/%s", name);
		else
			path = xstrdup(name);

		ret = bootscript_scan_path(blspec, path);
		if (ret > 0)
			found += ret;

		free(path);
	}

	return found;
}

/*
 * bootentries_collect - collect bootentries from an array of names
 */
static struct blspec *bootentries_collect(char *entries[], int num_entries)
{
	struct blspec *blspec;
	int i;

	blspec = blspec_alloc();

	if (IS_ENABLED(CONFIG_MENU))
		blspec->menu->display = asprintf("boot");

	if (!num_entries)
		bootscript_scan_path(blspec, "/env/boot");

	if (IS_ENABLED(CONFIG_BLSPEC) && !num_entries)
		blspec_scan_devices(blspec);

	for (i = 0; i < num_entries; i++)
		bootentry_parse_one(blspec, entries[i]);

	return blspec;
}

/*
 * bootsources_menu - show a menu from an array of names
 */
static void bootsources_menu(char *entries[], int num_entries)
{
	struct blspec *blspec = NULL;
	struct blspec_entry *entry, *entry_default;
	struct menu_entry *back_entry;

	if (!IS_ENABLED(CONFIG_MENU)) {
		printf("no menu support available\n");
		return;
	}

	blspec = bootentries_collect(entries, num_entries);
	if (blspec)
		return;

	entry_default = blspec_entry_default(blspec);

	blspec_for_each_entry(blspec, entry) {
		entry->me.action = bootsource_action;
		menu_add_entry(blspec->menu, &entry->me);
		if (entry == entry_default)
			menu_set_selected_entry(blspec->menu, &entry->me);
	}

	back_entry = xzalloc(sizeof(*back_entry));
	back_entry->display = "back";
	back_entry->type = MENU_ENTRY_NORMAL;
	back_entry->non_re_ent = 1;
	menu_add_entry(blspec->menu, back_entry);

	if (timeout >= 0)
		blspec->menu->auto_select = timeout;

	menu_show(blspec->menu);

	free(back_entry);

	blspec_free(blspec);
}

/*
 * bootsources_list - list boot entries from an array of names
 */
static void bootsources_list(char *entries[], int num_entries)
{
	struct blspec *blspec;
	struct blspec_entry *entry, *entry_default;

	blspec = bootentries_collect(entries, num_entries);
	if (!blspec)
		return;

	entry_default = blspec_entry_default(blspec);

	printf("  %-20s %-20s  %s\n", "device", "hwdevice", "title");
	printf("  %-20s %-20s  %s\n", "------", "--------", "-----");

	blspec_for_each_entry(blspec, entry) {
		if (entry == entry_default)
			printf("* ");
		else
			printf("  ");

		if (entry->scriptpath)
			printf("%-40s   %s\n", basename(entry->scriptpath), entry->me.display);
		else
			printf("%s\n", entry->me.display);
	}

	blspec_free(blspec);
}

/*
 * boot a script or a bootspec entry. 'name' can be:
 * - a filename under /env/boot/
 * - a full path to a boot script
 * - a device name
 * - a partition name under /dev/
 * - a full path to a directory which
 *   - contains boot scripts, or
 *   - contains a loader/entries/ directory containing bootspec entries
 *
 * Returns a negative error on failure, or 0 on a successful dryrun boot.
 */
static int boot(const char *name)
{
	struct blspec *blspec;
	struct blspec_entry *entry, *entry_default;
	int ret;

	blspec = blspec_alloc();
	ret = bootentry_parse_one(blspec, name);
	if (ret < 0)
		return ret;

	if (!ret) {
		printf("Nothing bootable found on %s\n", name);
		return -ENOENT;
	}

	entry_default = blspec_entry_default(blspec);
	if (entry_default) {
		ret = boot_entry(entry_default);
		if (!ret)
			return ret;
		printf("booting %s failed: %s\n", entry_default->me.display,
				strerror(-ret));
	}

	blspec_for_each_entry(blspec, entry) {
		if (entry == entry_default)
			continue;

		printf("booting %s\n", entry->me.display);
		ret = boot_entry(entry);
		if (!ret)
			break;
		printf("booting %s failed: %s\n", entry->me.display, strerror(-ret));
	}

	return ret;
}

static int do_boot(int argc, char *argv[])
{
	char *freep = NULL;
	int opt, ret = 0, do_list = 0, do_menu = 0;
	char **sources;
	int num_sources;
	int i;

	verbose = 0;
	dryrun = 0;
	timeout = -1;

	while ((opt = getopt(argc, argv, "vldmt:")) > 0) {
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
		}
	}

	if (optind < argc) {
		num_sources = argc - optind;
		sources = xmemdup(&argv[optind], sizeof(char *) * num_sources);
	} else {
		const char *def;
		char *sep;

		def = getenv("global.boot.default");
		if (!def)
			return 0;

		sep = freep = xstrdup(def);

		num_sources = 0;

		while (1) {
			num_sources++;

			sep = strchr(sep, ' ');
			if (!sep)
				break;
			sep++;
		}

		sources = xmalloc(sizeof(char *) * num_sources);

		sep = freep;

		for (i = 0; i < num_sources; i++) {
			sources[i] = sep;
			sep = strchr(sep, ' ');
			if (sep)
				*sep = 0;
			sep++;
		}
	}

	if (do_list) {
		bootsources_list(sources, num_sources);
		goto out;
	}

	if (do_menu) {
		bootsources_menu(sources, num_sources);
		goto out;
	}

	for (i = 0; i < num_sources; i++) {
		ret = boot(sources[i]);
		if (!ret)
			break;
	}

out:
	free(sources);
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
