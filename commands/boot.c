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
#include <boot.h>
#include <menu.h>
#include <fs.h>
#include <complete.h>

#include <linux/stat.h>

static int boot_script(char *path);

static int verbose;
static int dryrun;

static void bootsource_action(struct menu *m, struct menu_entry *me)
{
	struct blspec_entry *be = container_of(me, struct blspec_entry, me);
	int ret;

	if (be->scriptpath) {
		ret = boot_script(be->scriptpath);
	} else {
		if (IS_ENABLED(CONFIG_BLSPEC))
			ret = blspec_boot(be, 0, 0);
		else
			ret = -ENOSYS;
	}

	if (ret)
		printf("Booting failed with: %s\n", strerror(-ret));

	printf("Press any key to continue\n");

	read_key();
}

static int bootsources_menu_env_entries(struct blspec *blspec)
{
	const char *path = "/env/boot";
	DIR *dir;
	struct dirent *d;
	struct blspec_entry *be;

	dir = opendir(path);
	if (!dir)
		return -errno;

	while ((d = readdir(dir))) {

		if (*d->d_name == '.')
			continue;

		be = blspec_entry_alloc(blspec);
		be->me.type = MENU_ENTRY_NORMAL;
		be->scriptpath = asprintf("/env/boot/%s", d->d_name);
		be->me.display = xstrdup(d->d_name);
	}

	closedir(dir);

	return 0;
}

static struct blspec *bootentries_collect(void)
{
	struct blspec *blspec;

	blspec = blspec_alloc();
	blspec->menu->display = asprintf("boot");
	bootsources_menu_env_entries(blspec);
	if (IS_ENABLED(CONFIG_BLSPEC))
		blspec_scan_devices(blspec);
	return blspec;
}

static void bootsources_menu(void)
{
	struct blspec *blspec = NULL;
	struct blspec_entry *entry;
	struct menu_entry *back_entry;

	if (!IS_ENABLED(CONFIG_MENU)) {
		printf("no menu support available\n");
		return;
	}

	blspec = bootentries_collect();

	blspec_for_each_entry(blspec, entry) {
		entry->me.action = bootsource_action;
		menu_add_entry(blspec->menu, &entry->me);
	}

	back_entry = xzalloc(sizeof(*back_entry));
	back_entry->display = "back";
	back_entry->type = MENU_ENTRY_NORMAL;
	back_entry->non_re_ent = 1;
	menu_add_entry(blspec->menu, back_entry);

	menu_show(blspec->menu);

	free(back_entry);

	blspec_free(blspec);
}

static void bootsources_list(void)
{
	struct blspec *blspec;
	struct blspec_entry *entry;

	blspec = bootentries_collect();

	printf("\nBootscripts:\n\n");
	printf("%-40s   %-20s\n", "name", "title");
	printf("%-40s   %-20s\n", "----", "-----");

	blspec_for_each_entry(blspec, entry) {
		if (entry->scriptpath)
			printf("%-40s   %s\n", basename(entry->scriptpath), entry->me.display);
	}

	if (!IS_ENABLED(CONFIG_BLSPEC))
		return;

	printf("\nBootloader spec entries:\n\n");
	printf("%-20s %-20s  %s\n", "device", "hwdevice", "title");
	printf("%-20s %-20s  %s\n", "------", "--------", "-----");

	blspec_for_each_entry(blspec, entry)
		if (!entry->scriptpath)
			printf("%s\n", entry->me.display);

	blspec_free(blspec);
}

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

	printf("booting %s...\n", basename(path));

	globalvar_set_match("linux.bootargs.dyn.", "");
	globalvar_set_match("bootm.", "");

	ret = run_command(path, 0);
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

/*
 * boot a script. 'name' can either be a filename under /env/boot/,
 * a full path to a boot script or a path to a directory. This function
 * returns a negative error on failure, or 0 on a successful dryrun boot.
 */
static int boot(const char *name)
{
	char *path;
	DIR *dir;
	struct dirent *d;
	struct stat s;
	int ret;

	if (*name == '/')
		path = xstrdup(name);
	else
		path = asprintf("/env/boot/%s", name);

	ret = stat(path, &s);
	if (ret) {
		if (!IS_ENABLED(CONFIG_BLSPEC)) {
			pr_err("%s: %s\n", path, strerror(-ret));
			goto out;
		}

		ret = blspec_boot_hwdevice(name, verbose, dryrun);
		pr_err("%s: %s\n", name, strerror(-ret));
		goto out;
	}

	if (S_ISREG(s.st_mode)) {
		ret = boot_script(path);
		goto out;
	}

	dir = opendir(path);
	if (!dir) {
		ret = -errno;
		printf("cannot open %s: %s\n", path, strerror(-errno));
		goto out;
	}

	while ((d = readdir(dir))) {
		char *file;
		struct stat s;

		if (*d->d_name == '.')
			continue;

		file = asprintf("%s/%s", path, d->d_name);

		ret = stat(file, &s);
		if (ret) {
			free(file);
			continue;
		}

		if (!S_ISREG(s.st_mode)) {
			free(file);
			continue;
		}

		ret = boot_script(file);

                free(file);

		if (!ret)
			break;
	}

	closedir(dir);
out:
	free(path);

	return ret;
}

static int do_boot(int argc, char *argv[])
{
	const char *sources = NULL;
	char *source, *freep;
	int opt, ret = 0, do_list = 0, do_menu = 0;

	verbose = 0;
	dryrun = 0;

	while ((opt = getopt(argc, argv, "vldm")) > 0) {
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
		}
	}

	if (do_list) {
		bootsources_list();
		return 0;
	}

	if (do_menu) {
		bootsources_menu();
		return 0;
	}

	if (optind < argc) {
		while (optind < argc) {
			source = argv[optind];
			optind++;
			ret = boot(source);
			if (!ret)
				break;
		}
		return ret;
	}

	sources = getenv("global.boot.default");
	if (!sources)
		return 0;

	freep = source = xstrdup(sources);

	while (1) {
		char *sep = strchr(source, ' ');
		if (sep)
			*sep = 0;
		ret = boot(source);
		if (!ret)
			break;

		if (sep)
			source = sep + 1;
		else
			break;
	}

	free(freep);

	return ret;
}

BAREBOX_CMD_HELP_START(boot)
BAREBOX_CMD_HELP_USAGE("boot [OPTIONS] [BOOTSRC...]\n")
BAREBOX_CMD_HELP_SHORT("Boot an operating system.\n")
BAREBOX_CMD_HELP_SHORT("[BOOTSRC...] can be:\n")
BAREBOX_CMD_HELP_SHORT("- a filename from /env/boot/\n")
BAREBOX_CMD_HELP_SHORT("- a full path to a file\n")
BAREBOX_CMD_HELP_SHORT("- a path to a directory. All files in this directory are treated\n")
BAREBOX_CMD_HELP_SHORT("  as boot scripts.\n")
BAREBOX_CMD_HELP_SHORT("Multiple bootsources may be given which are probed in order until\n")
BAREBOX_CMD_HELP_SHORT("one succeeds.\n")
BAREBOX_CMD_HELP_SHORT("\nOptions:\n")
BAREBOX_CMD_HELP_OPT  ("-v","Increase verbosity\n")
BAREBOX_CMD_HELP_OPT  ("-d","Dryrun. See what happens but do no actually boot\n")
BAREBOX_CMD_HELP_OPT  ("-l","List available boot sources\n")
BAREBOX_CMD_HELP_OPT  ("-m","Show a menu with boot options\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(boot)
	.cmd	= do_boot,
	.usage		= "boot the machine",
	BAREBOX_CMD_HELP(cmd_boot_help)
BAREBOX_CMD_END

BAREBOX_MAGICVAR_NAMED(global_boot_default, global.boot.default, "default boot order");
