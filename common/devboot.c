// SPDX-License-Identifier: GPL-2.0-only
/*
 * devboot - boot with overrides configured by a script on the network
 *
 * See Documentation/user/devboot.rst
 */

#define pr_fmt(fmt) "devboot: " fmt

#include <boot.h>
#include <bootm.h>
#include <bootm-overrides.h>
#include <devboot.h>
#include <environment.h>
#include <globalvar.h>
#include <init.h>
#include <libfile.h>
#include <magicvar.h>
#include <net.h>
#include <shell.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/err.h>
#include <linux/stat.h>

struct devboot {
	struct globalvar_scope *scope;
	char *oldcwd;
};

static int devboot_default;
static struct devboot *devboot_active;

bool devboot_enabled(void)
{
	return devboot_default;
}

/*
 * Find the devboot script in the current directory, preferring a
 * board-specific over an architecture-specific one.
 */
static char *devboot_find_script(const char *fetchdir)
{
	const char *user = globalvar_get("user") ?: "";
	char *board, *arch;

	board = xasprintf("%s-devboot-%s", user, globalvar_get("hostname") ?: "");
	if (file_exists_type(board, S_IFREG))
		return board;

	arch = xasprintf("%s-devboot-%s", user, globalvar_get("arch") ?: "");
	if (file_exists_type(arch, S_IFREG)) {
		free(board);
		return arch;
	}

	pr_err("Neither %s/%s nor %s/%s found\n", fetchdir, board, fetchdir, arch);
	pr_err("Create one of them and set as needed:\n");
	pr_err("  devboot_image, devboot_initrd, devboot_oftree\n");

	free(board);
	free(arch);

	return NULL;
}

/*
 * Take over a variable set by the devboot script as override, unless it's
 * already given on the command line. Unset variables are ignored, while
 * empty ones are taken over, so that e.g. an empty devboot_oftree discards
 * the boot entry's device tree.
 */
static void devboot_take_override(const char **override, const char *var)
{
	const char *val;

	val = getenv(var);
	if (val && !*override)
		*override = xstrdup(val);
}

static int devboot_source(const char *fetchdir, struct bootm_overrides *overrides)
{
	char *script, *path;
	char *argv[2] = {};
	int ret;

	script = devboot_find_script(fetchdir);
	if (!script)
		return -ENOENT;

	path = xasprintf("%s/%s", fetchdir, script);
	pr_info("Sourcing %s\n", path);

	/* Local variables of the script vanish with the context */
	env_push_context();

	argv[0] = path;
	ret = source_script(path, 1, argv);
	if (ret) {
		pr_err("%s failed with %d\n", path, ret);
		ret = -EINVAL;
	} else {
		devboot_take_override(&overrides->os_file, "devboot_image");
		devboot_take_override(&overrides->oftree_file, "devboot_oftree");
		devboot_take_override(&overrides->initrd_file, "devboot_initrd");
	}

	env_pop_context();
	free(path);
	free(script);

	return ret;
}

/**
 * devboot_start - prepare a boot with devboot
 * @overrides: overrides given on the command line, completed by the ones
 *             the devboot script sets
 *
 * Brings up the network, changes into global.net.fetchdir and sources the
 * devboot script found there. Changes the script makes to globalvars and
 * the working directory persist until devboot_finish(), so relative paths
 * in the overrides are resolved against the fetch directory.
 *
 * A boot started while devboot is active, e.g. by a boot script, doesn't
 * source the script again. It inherits the overrides from the boot entry
 * it is started from.
 *
 * Return: a handle for devboot_finish(), NULL if devboot was not started,
 * or an error pointer
 */
struct devboot *devboot_start(struct bootm_overrides *overrides)
{
	struct devboot *db;
	const char *fetchdir;
	int ret;

	if (devboot_active)
		return NULL;

	/*
	 * Overrides are ignored then, so don't source a script from the
	 * network only to drop what it sets
	 */
	if (bootm_signed_images_are_forced()) {
		pr_err("Signed images are forced, devboot is not possible\n");
		return ERR_PTR(-EPERM);
	}

	fetchdir = globalvar_get("net.fetchdir");
	if (!fetchdir || !*fetchdir) {
		pr_err("global.net.fetchdir is not set\n");
		return ERR_PTR(-EINVAL);
	}

	db = xzalloc(sizeof(*db));
	db->scope = boot_globalvar_scope_push();
	devboot_active = db;

	/* global.net.server and global.hostname may be set by DHCP */
	if (IS_ENABLED(CONFIG_NET_IFUP))
		ifup_all(IFUP_FLAG_PARALLEL | IFUP_FLAG_UNTIL_NET_SERVER);

	db->oldcwd = pushd(fetchdir);
	if (!db->oldcwd) {
		pr_err("Cannot change to %s\n", fetchdir);
		ret = -ENOENT;
		goto err;
	}

	ret = devboot_source(fetchdir, overrides);
	if (ret)
		goto err;

	return db;
err:
	devboot_finish(db);
	return ERR_PTR(ret);
}

/**
 * devboot_finish - revert the changes of devboot_start()
 * @db: handle returned by devboot_start(), may be NULL or an error pointer
 */
void devboot_finish(struct devboot *db)
{
	if (IS_ERR_OR_NULL(db))
		return;

	popd(db->oldcwd);
	globalvar_scope_pop(db->scope);
	free(db);

	devboot_active = NULL;
}

static int devboot_init(void)
{
	return globalvar_add_simple_bool("boot.devboot", &devboot_default);
}
device_initcall(devboot_init);

BAREBOX_MAGICVAR(global.boot.devboot,
		 "Boot with devboot overrides by default, see boot -D");
