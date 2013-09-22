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
#include <common.h>
#include <getopt.h>
#include <libgen.h>
#include <malloc.h>
#include <boot.h>
#include <fs.h>

#include <linux/stat.h>

static int verbose;
static int dryrun;

static void bootsources_list(void)
{
	DIR *dir;
	struct dirent *d;
	const char *path = "/env/boot";

	dir = opendir(path);
	if (!dir) {
		printf("cannot open %s: %s\n", path, strerror(-errno));
		return;
	}

	printf("Bootsources: ");

	while ((d = readdir(dir))) {
		if (*d->d_name == '.')
			continue;

		printf("%s ", d->d_name);
	}

	printf("\n");

	closedir(dir);
}

static const char *getenv_or_null(const char *var)
{
	const char *val = getenv(var);

	if (val && *val)
		return val;
	return NULL;
}

/*
 * Start a single boot script. 'path' is a full path to a boot script.
 */
static int boot_script(char *path)
{
	struct bootm_data data = {};
	int ret;

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
	data.oftree_file = getenv_or_null("global.bootm.oftree");
	data.os_file = getenv_or_null("global.bootm.image");
	data.os_address = getenv_loadaddr("global.bootm.image.loadaddr");
	data.initrd_address = getenv_loadaddr("global.bootm.initrd.loadaddr");
	data.initrd_file = getenv_or_null("global.bootm.initrd");
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
		pr_err("%s: %s\n", path, strerror(-ret));
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
	int opt, ret = 0, do_list = 0;

	verbose = 0;
	dryrun = 0;

	while ((opt = getopt(argc, argv, "vld")) > 0) {
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
		}
	}

	if (do_list) {
		bootsources_list();
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
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(boot)
	.cmd	= do_boot,
	.usage		= "boot the machine",
	BAREBOX_CMD_HELP(cmd_boot_help)
BAREBOX_CMD_END

BAREBOX_MAGICVAR_NAMED(global_boot_default, global.boot.default, "default boot order");
