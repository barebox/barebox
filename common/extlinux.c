// SPDX-License-Identifier: GPL-2.0+
/* SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru> */

#define pr_fmt(fmt)     "extlinux: " fmt

#include <boot.h>
#include <bootm.h>
#include <bootscan.h>
#include <common.h>
#include <environment.h>
#include <fs.h>
#include <globalvar.h>
#include <libfile.h>
#include <libgen.h>
#include <string.h>

struct extlinux_entry {
	struct bootentry entry;
	char *rootpath;
	char *label;
	char *kernel;
	char *initrd;
	char *fdtdir;
	char *fdt;
	char *append;
};

static int extlinux_boot(struct bootentry *entry, int verbose, int dryrun)
{
	struct extlinux_entry *e =
		container_of(entry, struct extlinux_entry, entry);
	char *kernel_abs, *initrd_abs = NULL, *fdt_abs = NULL;
	struct bootm_data data = {};
	int ret;

	bootm_data_init_defaults(&data);

	data.dryrun = max_t(int, dryrun, data.dryrun);
	data.verbose = max(verbose, data.verbose);

	kernel_abs = basprintf("%s/%s", e->rootpath, e->kernel);
	data.os_file = kernel_abs;

	if (e->initrd) {
		initrd_abs = basprintf("%s/%s", e->rootpath, e->initrd);
		data.initrd_file = initrd_abs;
	}

	if (e->fdt) {
		char *fdtdir = e->fdtdir ? : e->rootpath;

		fdt_abs = basprintf("%s/%s", fdtdir, e->fdt);
		data.oftree_file = fdt_abs;
	}

	if (e->append)
		globalvar_add_simple("linux.bootargs.dyn.bootentries",
				     e->append);

	pr_info("Booting extlinux label '%s'\n", e->label);

	ret = bootm_entry(entry, &data);
	if (ret)
		pr_err("bootm failed: %pe\n", ERR_PTR(ret));

	free(kernel_abs);
	free(initrd_abs);
	free(fdt_abs);

	return ret;
}

static void extlinux_entry_free(struct bootentry *entry)
{
	struct extlinux_entry *e =
		container_of(entry, struct extlinux_entry, entry);

	free(e->rootpath);
	free(e->label);
	free(e->kernel);
	free(e->initrd);
	free(e->fdtdir);
	free(e->fdt);
	free(e->append);
	free(e);
}

/*
 * Parse extlinux.conf. Only the entry pointed to by the DEFAULT keyword
 * is extracted; all other LABEL sections are ignored.
 */
static struct extlinux_entry *parse_extlinux_conf(const char *abspath,
						  const char *rootpath)
{
	char *buf, *bufptr, *line, *default_label = NULL;
	struct extlinux_entry *entry = NULL;

	bufptr = read_file(abspath, NULL);
	if (!bufptr)
		return ERR_PTR(-errno);

	buf = bufptr;
	while ((line = strsep(&buf, "\n\r")) != NULL) {
		char *key, *val;

		line = skip_spaces(line);

		if (*line == '#' || *line == '\0')
			continue;

		key = strsep(&line, " \t");
		val = isempty(line) ? NULL : skip_spaces(line);
		if (!key || !val)
			continue;

		if (!default_label) {
			if (!strcasecmp(key, "DEFAULT"))
				default_label = xstrdup(val);

			continue;
		}

		if (!strcasecmp(key, "LABEL")) {
			if (!strcmp(val, default_label)) {
				entry = xzalloc(sizeof(*entry));
				entry->label = xstrdup(val);
				entry->rootpath = dirname(xstrdup(abspath));
			} else if (entry) {
				break;
			}
			continue;
		}

		if (entry) {
			if (!strcasecmp(key, "KERNEL"))
				entry->kernel = xstrdup(val);
			else if (!strcasecmp(key, "INITRD"))
				entry->initrd = xstrdup(val);
			else if (!strcasecmp(key, "FDTDIR"))
				entry->fdtdir = xstrdup(val);
			else if (!strcasecmp(key, "FDT"))
				entry->fdt = xstrdup(val);
			else if (!strcasecmp(key, "APPEND"))
				entry->append = xstrdup(val);
			else
				pr_warn("Unhandled key: %s\n", key);
		}
	}

	free(default_label);
	free(bufptr);

	if (!entry || !entry->kernel) {
		if (entry)
			extlinux_entry_free(&entry->entry);
		return ERR_PTR(-EINVAL);
	}

	return entry;
}

static int _extlinux_scan_file(struct bootscanner *scanner,
			       struct bootentries *bootentries,
			       const char *configname,
			       const char *rootpath)
{
	struct extlinux_entry *e;

	if (!strends(configname, "extlinux.conf"))
		return 0;

	e = parse_extlinux_conf(configname, rootpath);
	if (IS_ERR(e))
		return PTR_ERR(e);

	e->entry.boot = extlinux_boot;
	e->entry.release = extlinux_entry_free;
	e->entry.path = xstrdup_const(configname);
	e->entry.title = basprintf("extlinux: %s", e->label);
	e->entry.description = basprintf("extlinux entry \'%s\" on %s",
					 e->label, rootpath);
	e->entry.me.type = MENU_ENTRY_NORMAL;

	bootentries_add_entry(bootentries, &e->entry);

	return 1;
}

static int extlinux_scan_file(struct bootscanner *scanner,
			      struct bootentries *bootentries,
			      const char *configname)
{
	const char *rootpath = get_mounted_path(configname);

	if (IS_ERR(rootpath))
		return PTR_ERR(rootpath);

	return _extlinux_scan_file(scanner, bootentries, configname, rootpath);
}

static int extlinux_scan_directory(struct bootscanner *scanner,
				   struct bootentries *bootentries,
				   const char *rootpath)
{
	char *path;
	struct stat s;
	int ret;

	path = basprintf("%s/boot/extlinux/extlinux.conf", rootpath);
	ret = stat(path, &s);
	if (!ret && S_ISREG(s.st_mode))
		ret = _extlinux_scan_file(scanner, bootentries, path, rootpath);
	free(path);
	if (ret > 0)
		return ret;

	path = basprintf("%s/extlinux/extlinux.conf", rootpath);
	ret = stat(path, &s);
	if (!ret && S_ISREG(s.st_mode))
		ret = _extlinux_scan_file(scanner, bootentries, path, rootpath);
	free(path);

	return ret;
}

static struct bootscanner extlinux_scanner = {
	.name = "extlinux",
	.scan_file = extlinux_scan_file,
	.scan_directory = extlinux_scan_directory,
};

static int extlinux_generate(struct bootentries *bootentries, const char *name)
{
	return bootentry_scan_generate(&extlinux_scanner, bootentries, name);
}

static struct bootentry_provider extlinux_provider = {
	.name = "extlinux",
	.generate = extlinux_generate,
	.priority = -25,
};

static int extlinux_init(void)
{
	return bootentry_register_provider(&extlinux_provider);
}
device_initcall(extlinux_init);
