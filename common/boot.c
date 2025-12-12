// SPDX-License-Identifier: GPL-2.0-only

#include <boot.h>
#include <globalvar.h>
#include <magicvar.h>
#include <watchdog.h>
#include <command.h>
#include <readkey.h>
#include <common.h>
#include <libgen.h>
#include <bootm.h>
#include <glob.h>
#include <init.h>
#include <menu.h>
#include <unistd.h>
#include <libfile.h>
#include <net.h>
#include <fs.h>

#include <linux/stat.h>

int bootentries_add_entry(struct bootentries *entries, struct bootentry *entry)
{
	list_add_tail(&entry->list, &entries->entries);

	return 0;
}

struct bootentries *bootentries_alloc(void)
{
	struct bootentries *bootentries;

	bootentries = xzalloc(sizeof(*bootentries));
	INIT_LIST_HEAD(&bootentries->entries);

	if (IS_ENABLED(CONFIG_MENU)) {
		bootentries->menu = menu_alloc();
		menu_add_title(bootentries->menu, "boot");
	}

	return bootentries;
}

void bootentries_free(struct bootentries *bootentries)
{
	struct bootentry *be, *tmp;

	list_for_each_entry_safe(be, tmp, &bootentries->entries, list) {
		list_del(&be->list);
		free_const(be->title);
		free(be->description);
		free_const(be->me.display);
		be->release(be);
	}

	if (IS_ENABLED(CONFIG_MENU) && bootentries->menu) {
		int i;
		for (i = 0; i < bootentries->menu->display_lines; i++)
			free_const(bootentries->menu->display[i]);
		free_const(bootentries->menu->display);
		free(bootentries->menu);
	}

	free(bootentries);
}

struct bootentry_script {
	struct bootentry entry;
	const char *scriptpath;
};

/*
 * Start a single boot script. 'path' is a full path to a boot script.
 */
static int bootscript_boot(struct bootentry *entry, int verbose, int dryrun)
{
	struct bootentry_script *bs = container_of(entry, struct bootentry_script, entry);
	int ret;

	struct bootm_data backup = {}, data = {};

	if (dryrun == 1) {
		printf("Would run %s\n", bs->scriptpath);
		return 0;
	}

	bootm_data_init_defaults(&backup);

	globalvar_add_simple("linux.bootargs.dyn.ip", NULL);
	globalvar_add_simple("linux.bootargs.dyn.root", NULL);

	ret = run_command(bs->scriptpath);
	if (ret) {
		pr_err("Running script '%s' failed: %pe\n", bs->scriptpath, ERR_PTR(ret));
		goto out;
	}

	bootm_data_init_defaults(&data);

	if (verbose)
		data.verbose = verbose;
	if (dryrun >= 2)
		data.dryrun = dryrun - 1;

	ret = bootm_boot(&data);
out:
	bootm_data_restore_defaults(&backup);
	return ret;
}

static unsigned int boot_watchdog_timeout;

void boot_set_watchdog_timeout(unsigned int timeout)
{
	boot_watchdog_timeout = timeout;
}

static struct watchdog *boot_enabled_watchdog;

struct watchdog *boot_get_enabled_watchdog(void)
{
	return boot_enabled_watchdog;
}

static char *global_boot_default;

void boot_set_default(const char *boot_default)
{
	free(global_boot_default);
	global_boot_default = xstrdup(boot_default);
}

static char *global_user;

static int init_boot(void)
{
	if (!global_boot_default)
		global_boot_default = xstrdup(
			IF_ENABLED(CONFIG_BOOT_DEFAULTS, "bootsource ")
			IF_ENABLED(CONFIG_BOOT_DEFAULTS, "storage.builtin ")
			IF_ENABLED(CONFIG_BOOT_DEFAULTS, "storage.removable ")
			"net"
		);

	globalvar_add_simple_string("boot.default", &global_boot_default);
	globalvar_add_simple_int("boot.watchdog_timeout",
				 &boot_watchdog_timeout, "%u");
	globalvar_add_simple("linux.bootargs.base", NULL);
	global_user = xstrdup("none");
	globalvar_add_simple_string("user", &global_user);

	return 0;
}
late_initcall(init_boot);

BAREBOX_MAGICVAR(global.boot.watchdog_timeout,
		"Watchdog enable timeout in seconds before booting");

int boot_entry(struct bootentry *be, int verbose, int dryrun)
{
	int ret;

	pr_info("Booting entry '%s'\n", be->title);

	if (IS_ENABLED(CONFIG_WATCHDOG) && boot_watchdog_timeout) {
		boot_enabled_watchdog = watchdog_get_default();

		ret = watchdog_set_timeout(boot_enabled_watchdog, boot_watchdog_timeout);
		if (ret) {
			pr_warn("Failed to enable watchdog: %pe\n", ERR_PTR(ret));
			boot_enabled_watchdog = NULL;
		}
	}

	bootm_set_overrides(&be->overrides);

	ret = be->boot(be, verbose, dryrun);
	if (ret && ret != -ENOMEDIUM)
		pr_err("Booting entry '%s' failed: %pe\n", be->title, ERR_PTR(ret));

	bootm_set_overrides(NULL);

	globalvar_set_match("linux.bootargs.dyn.", "");

	return ret;
}

static void bootsource_action(struct menu *m, struct menu_entry *me)
{
	struct bootentry *be = container_of(me, struct bootentry, me);

	boot_entry(be, 0, 0);

	printf("Press any key to continue\n");

	read_key();
}

static void bootscript_entry_release(struct bootentry *entry)
{
	struct bootentry_script *bs = container_of(entry, struct bootentry_script, entry);

	free_const(bs->scriptpath);
	free(bs);
}

/*
 * bootscript_create_entry - create a boot entry from a script name
 */
static int bootscript_create_entry(struct bootentries *bootentries, const char *name)
{
	struct bootentry_script *bs;
	enum filetype type;
	int ret;

	ret = file_name_detect_type(name, &type);
	if (ret)
		return ret;

	if (type != filetype_sh)
		return -EINVAL;

	bs = xzalloc(sizeof(*bs));
	bs->entry.me.type = MENU_ENTRY_NORMAL;
	bs->entry.release = bootscript_entry_release;
	bs->entry.boot = bootscript_boot;
	bs->scriptpath = xstrdup_const(name);
	bs->entry.title = xstrdup_const(kbasename(bs->scriptpath));
	bs->entry.description = basprintf("script: %s", name);
	bootentries_add_entry(bootentries, &bs->entry);

	return 0;
}

/*
 * bootscript_scan_path - create boot entries from a path
 *
 * path can either be a full path to a bootscript or a full path to a directory
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

static LIST_HEAD(bootentry_providers);

int bootentry_register_provider(struct bootentry_provider *p)
{
	list_add(&p->list, &bootentry_providers);
	return 0;
}

/*
 * nfs_find_mountpath - Check if a given url is already mounted
 */
static const char *nfs_find_mountpath(const char *nfshostpath)
{
	struct fs_device *fsdev;

	for_each_fs_device(fsdev) {
		if (fsdev->backingstore && !strcmp(fsdev->backingstore, nfshostpath))
			return fsdev->path;
	}

	return NULL;
}

/*
 * parse_nfs_url - check for nfs:// style url
 *
 * Check if the passed string is a NFS url and if yes, mount the
 * NFS and return the path we have mounted to.
 */
static char *parse_nfs_url(const char *url)
{
	char *sep, *str, *host, *port, *path;
	char *mountpath = NULL, *hostpath = NULL, *options = NULL;
	const char *prevpath;
	IPaddr_t ip;
	int ret;

	if (!IS_ENABLED(CONFIG_FS_NFS))
		return NULL;

	if (strncmp(url, "nfs://", 6))
		return NULL;

	url += 6;

	str = xstrdup(url);

	host = str;

	sep = strchr(str, '/');
	if (!sep) {
		ret = -EINVAL;
		goto out;
	}

	*sep++ = 0;

	path = sep;

	port = strchr(host, ':');
	if (port)
		*port++ = 0;

	ret = ifup_all(0);
	if (ret) {
		pr_err("Failed to bring up networking\n");
		goto out;
	}

	ret = resolv(host, &ip);
	if (ret) {
		pr_err("Cannot resolve \"%s\": %s\n", host, strerror(-ret));
		goto out;
	}

	hostpath = basprintf("%pI4:%s", &ip, path);

	prevpath = nfs_find_mountpath(hostpath);

	if (prevpath) {
		mountpath = xstrdup(prevpath);
	} else {
		mountpath = basprintf("/mnt/nfs-%s-bootentries-%08x", host,
					random32());
		if (port)
			options = basprintf("mountport=%s,port=%s", port,
					      port);

		ret = make_directory(mountpath);
		if (ret)
			goto out;

		pr_debug("host: %s port: %s path: %s\n", host, port, path);
		pr_debug("hostpath: %s mountpath: %s options: %s\n", hostpath, mountpath, options);

		ret = mount(hostpath, "nfs", mountpath, options);
		if (ret)
			goto out;
	}

	ret = 0;

out:
	free(str);
	free(hostpath);
	free(options);

	if (ret)
		free(mountpath);

	return ret ? NULL : mountpath;
}


/*
 * bootentry_create_from_name - create boot entries from a name
 *
 * name can be:
 * - a name of a boot script under /env/boot
 * - a full path of a boot script
 * - a full path of a bootloader spec entry
 * - a device name
 * - a cdev name
 * - a full path of a directory containing bootloader spec entries
 * - a full path of a directory containing bootscripts
 * - a nfs:// path
 *
 * Returns the number of entries found or a negative error code.
 */
int bootentry_create_from_name(struct bootentries *bootentries,
				      const char *name)
{
	struct bootentry_provider *p;
	int found = 0, ret;
	char *nfspath;

	nfspath = parse_nfs_url(name);
	if (nfspath)
		name = nfspath;

	list_for_each_entry(p, &bootentry_providers, list) {
		ret = p->generate(bootentries, name);
		if (ret > 0)
			found += ret;
	}

	free(nfspath);

	if (IS_ENABLED(CONFIG_COMMAND_SUPPORT) && !found) {
		const char *path;

		if (*name != '/')
			path = basprintf("/env/boot/%s", name);
		else
			path = xstrdup_const(name);

		ret = bootscript_scan_path(bootentries, path);
		if (ret > 0)
			found += ret;

		free_const(path);
	}

	return found;
}

/*
 * bootsources_menu - show a menu from an array of names
 */
void bootsources_menu(struct bootentries *bootentries,
		      unsigned default_entry, int timeout)
{
	struct bootentry *entry;
	struct menu_entry *back_entry;
	int i = 1;

	if (!IS_ENABLED(CONFIG_MENU)) {
		pr_warn("no menu support available\n");
		return;
	}

	bootentries_for_each_entry(bootentries, entry) {
		if (!entry->me.display)
			entry->me.display = xstrdup_const(entry->title);
		entry->me.action = bootsource_action;
		menu_add_entry(bootentries->menu, &entry->me);

		if (i++ == default_entry)
			bootentries->menu->selected = &entry->me;
	}

	back_entry = xzalloc(sizeof(*back_entry));
	back_entry->display = "back";
	back_entry->type = MENU_ENTRY_NORMAL;
	back_entry->non_re_ent = 1;
	menu_add_entry(bootentries->menu, back_entry);

	if (i == default_entry)
		bootentries->menu->selected = back_entry;

	if (timeout >= 0)
		bootentries->menu->auto_select = timeout;

	menu_show(bootentries->menu);

	menu_remove_entry(bootentries->menu, back_entry);
	free(back_entry);
}

/*
 * bootsources_list - list boot entries from an array of names
 */
void bootsources_list(struct bootentries *bootentries)
{
	struct bootentry *entry;

	printf("title\n------\n");

	bootentries_for_each_entry(bootentries, entry)
		printf("%s\n\t%s\n", entry->title, entry->description);
}

BAREBOX_MAGICVAR(global.boot.default, "default boot order");
