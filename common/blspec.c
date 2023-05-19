// SPDX-License-Identifier: GPL-2.0-or-later
#define pr_fmt(fmt)  "blspec: " fmt

#include <environment.h>
#include <globalvar.h>
#include <firmware.h>
#include <readkey.h>
#include <common.h>
#include <driver.h>
#include <blspec.h>
#include <malloc.h>
#include <block.h>
#include <fcntl.h>
#include <libfile.h>
#include <libbb.h>
#include <init.h>
#include <bootm.h>
#include <glob.h>
#include <net.h>
#include <fs.h>
#include <of.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <mtd/ubi-user.h>

/*
 * blspec_entry_var_set - set a variable to a value
 */
int blspec_entry_var_set(struct blspec_entry *entry, const char *name,
		const char *val)
{
	return of_set_property(entry->node, name, val,
			val ? strlen(val) + 1 : 0, 1);
}

static int blspec_overlay_fixup(struct device_node *root, void *ctx)
{
	struct blspec_entry *entry = ctx;
	const char *overlays;
	char *overlay;
	char *sep, *freep;

	overlays = blspec_entry_var_get(entry, "devicetree-overlay");

	sep = freep = xstrdup(overlays);

	while ((overlay = strsep(&sep, " "))) {
		char *path;

		if (!*overlay)
			continue;

		path = basprintf("%s/%s", entry->rootpath, overlay);

		of_overlay_apply_file(root, path, false);

		free(path);
	}

	free(freep);

	return 0;
}

/*
 * blspec_boot - boot an entry
 *
 * This boots an entry. On success this function does not return.
 * In case of an error the error code is returned. This function may
 * return 0 in case of a successful dry run.
 */
static int blspec_boot(struct bootentry *be, int verbose, int dryrun)
{
	struct blspec_entry *entry = container_of(be, struct blspec_entry, entry);
	int ret;
	const char *abspath, *devicetree, *options, *initrd, *linuximage;
	const char *overlays;
	const char *appendroot;
	char *old_fws, *fws;
	struct bootm_data data = {
		.dryrun = dryrun,
	};

	globalvar_set_match("linux.bootargs.dyn.", "");
	globalvar_set("bootm.image", "");
	globalvar_set("bootm.oftree", "");
	globalvar_set("bootm.initrd", "");

	bootm_data_init_defaults(&data);

	data.verbose = max(verbose, data.verbose);

	devicetree = blspec_entry_var_get(entry, "devicetree");
	initrd = blspec_entry_var_get(entry, "initrd");
	options = blspec_entry_var_get(entry, "options");
	linuximage = blspec_entry_var_get(entry, "linux");
	overlays = blspec_entry_var_get(entry, "devicetree-overlay");

	if (entry->rootpath)
		abspath = entry->rootpath;
	else
		abspath = "";

	data.os_file = basprintf("%s/%s", abspath, linuximage);

	if (devicetree) {
		if (!strcmp(devicetree, "none")) {
			struct device_node *node = of_get_root_node();
			if (node)
				of_delete_node(node);
		} else {
			data.oftree_file = basprintf("%s/%s", abspath,
						       devicetree);
		}
	}

	if (overlays)
		of_register_fixup(blspec_overlay_fixup, entry);

	if (initrd)
		data.initrd_file = basprintf("%s/%s", abspath, initrd);

	globalvar_add_simple("linux.bootargs.dyn.bootentries", options);

	appendroot = blspec_entry_var_get(entry, "linux-appendroot");
	if (appendroot) {
		int val;

		ret = strtobool(appendroot, &val);
		if (ret) {
			pr_err("Invalid value \"%s\" for appendroot option\n",
			       appendroot);
			goto err_out;
		}
		data.appendroot = val;
	}

	pr_info("booting %s from %s\n", blspec_entry_var_get(entry, "title"),
			(entry->cdev && entry->cdev->dev) ?
			dev_name(entry->cdev->dev) : "none");

	of_overlay_set_basedir(abspath);

	old_fws = firmware_get_searchpath();
	if (old_fws && *old_fws)
		fws = basprintf("%s/lib/firmware:%s", abspath, old_fws);
	else
		fws = basprintf("%s/lib/firmware", abspath);
	firmware_set_searchpath(fws);
	free(fws);

	ret = bootm_boot(&data);
	if (ret)
		pr_err("Booting failed\n");

	if (overlays)
		of_unregister_fixup(blspec_overlay_fixup, entry);

	of_overlay_set_basedir("/");
	firmware_set_searchpath(old_fws);
	free(old_fws);

err_out:
	free((char *)data.oftree_file);
	free((char *)data.initrd_file);
	free((char *)data.os_file);

	return ret;
}

/*
 * blspec_entry_var_get - get the value of a variable
 */
const char *blspec_entry_var_get(struct blspec_entry *entry, const char *name)
{
	const char *str;
	int ret;

	ret = of_property_read_string(entry->node, name, &str);

	return ret ? NULL : str;
}

static void blspec_entry_free(struct bootentry *be)
{
	struct blspec_entry *entry = container_of(be, struct blspec_entry, entry);

	of_delete_node(entry->node);
	free(entry->configpath);
	free(entry->rootpath);
	free(entry);
}

static struct blspec_entry *blspec_entry_alloc(struct bootentries *bootentries)
{
	struct blspec_entry *entry;

	entry = xzalloc(sizeof(*entry));

	entry->node = of_new_node(NULL, NULL);
	entry->entry.release = blspec_entry_free;
	entry->entry.boot = blspec_boot;

	return entry;
}

/*
 * blspec_entry_open - open an entry given a path
 */
static struct blspec_entry *blspec_entry_open(struct bootentries *bootentries,
		const char *abspath)
{
	struct blspec_entry *entry;
	char *end, *line, *next;
	char *buf;

	pr_debug("%s: %s\n", __func__, abspath);

	buf = read_file(abspath, NULL);
	if (!buf)
		return ERR_PTR(-errno);

	entry = blspec_entry_alloc(bootentries);

	next = buf;

	while (next && *next) {
		char *name, *val;

		line = next;

		next = strchr(line, '\n');
		if (next) {
			*next = 0;
			next++;
		}

		if (*line == '#')
			continue;

		name = line;
		end = name;

		while (*end && (*end != ' ' && *end != '\t'))
			end++;

		if (!*end) {
			blspec_entry_var_set(entry, name, NULL);
			continue;
		}

		*end = 0;

		end++;

		while (*end == ' ' || *end == '\t')
			end++;

		if (!*end) {
			blspec_entry_var_set(entry, name, NULL);
			continue;
		}

		val = end;

		if (!strcmp(name, "options")) {
			/* If there was a previous "options" key given, prepend its value
			 * (as per spec). */
			const char *prev_val = blspec_entry_var_get(entry, name);
			if (prev_val) {
				char *opts = xasprintf("%s %s", prev_val, val);
				blspec_entry_var_set(entry, name, opts);
				free(opts);
				continue;
			}
		}

		blspec_entry_var_set(entry, name, val);
	}

	free(buf);

	return entry;
}

/*
 * is_blspec_entry - check if a bootentry is a blspec entry
 */
static inline bool is_blspec_entry(struct bootentry *entry)
{
	return entry->boot == blspec_boot;
}

/*
 * blspec_have_entry - check if we already have an entry with
 *                     a certain path
 */
static int blspec_have_entry(struct bootentries *bootentries, const char *path)
{
	struct bootentry *be;
	struct blspec_entry *e;

	list_for_each_entry(be, &bootentries->entries, list) {
		if (!is_blspec_entry(be))
			continue;
		e = container_of(be, struct blspec_entry, entry);
		if (e->configpath && !strcmp(e->configpath, path))
			return 1;
	}

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
					rand());
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
 * entry_is_of_compatible - check if a bootspec entry is compatible with
 *                          the current machine.
 *
 * returns true if the entry is compatible, false otherwise
 */
static bool entry_is_of_compatible(struct blspec_entry *entry)
{
	const char *devicetree;
	const char *abspath;
	int ret;
	struct device_node *root = NULL, *barebox_root;
	const char *compat;
	char *filename;

	/* If the entry doesn't specify a devicetree we are compatible */
	devicetree = blspec_entry_var_get(entry, "devicetree");
	if (!devicetree)
		return true;

	if (!strcmp(devicetree, "none"))
		return true;

	/* If we don't have a root node every entry is compatible */
	barebox_root = of_get_root_node();
	if (!barebox_root)
		return true;

	ret = of_property_read_string(barebox_root, "compatible", &compat);
	if (ret)
		return false;

	if (entry->rootpath)
		abspath = entry->rootpath;
	else
		abspath = "";

	filename = basprintf("%s/%s", abspath, devicetree);

	root = of_read_file(filename);
	if (IS_ERR(root)) {
		ret = false;
		root = NULL;
		goto out;
	}

	if (of_device_is_compatible(root, compat)) {
		ret = true;
		goto out;
	}

	pr_info("ignoring entry with incompatible devicetree \"%s\"\n",
			(char *)of_get_property(root, "compatible", NULL));

	ret = false;

out:
	of_delete_node(root);
	free(filename);

	return ret;
}

/*
 * entry_is_match_machine_id - check if a bootspec entry is match with
 *                            the machine id given by global variable.
 *
 * returns true if the entry is match, false otherwise
 */

static bool entry_is_match_machine_id(struct blspec_entry *entry)
{
	int ret = true;
	const char *env_machineid = getenv_nonempty("global.boot.machine_id");

	if (env_machineid) {
		const char *machineid = blspec_entry_var_get(entry, "machine-id");
		if (!machineid || strcmp(machineid, env_machineid)) {
			pr_debug("ignoring entry with mismatched machine-id " \
				"\"%s\" != \"%s\"\n", env_machineid, machineid);
			ret = false;
		}
	}

	return ret;
}

int blspec_scan_file(struct bootentries *bootentries, const char *root,
		     const char *configname)
{
	char *devname = NULL, *hwdevname = NULL;
	struct blspec_entry *entry;

	if (blspec_have_entry(bootentries, configname))
		return -EEXIST;

	entry = blspec_entry_open(bootentries, configname);
	if (IS_ERR(entry))
		return PTR_ERR(entry);

	root = root ?: get_mounted_path(configname);
	entry->rootpath = xstrdup(root);
	entry->configpath = xstrdup(configname);
	entry->cdev = get_cdev_by_mountpath(root);

	if (!entry_is_of_compatible(entry)) {
		blspec_entry_free(&entry->entry);
		return -ENODEV;
	}

	if (!entry_is_match_machine_id(entry)) {
		blspec_entry_free(&entry->entry);
		return -ENODEV;
	}

	if (entry->cdev && entry->cdev->dev) {
		devname = xstrdup(dev_name(entry->cdev->dev));
		if (entry->cdev->dev->parent)
			hwdevname = xstrdup(dev_name(entry->cdev->dev->parent));
	}

	entry->entry.title = xasprintf("%s (%s)", blspec_entry_var_get(entry, "title"),
				       configname);
	entry->entry.description = basprintf("blspec entry, device: %s hwdevice: %s",
					    devname ? devname : "none",
					    hwdevname ? hwdevname : "none");
	free(devname);
	free(hwdevname);

	entry->entry.me.type = MENU_ENTRY_NORMAL;
	entry->entry.release = blspec_entry_free;

	bootentries_add_entry(bootentries, &entry->entry);
	return 1;
}

/*
 * blspec_scan_directory - scan over a directory
 *
 * Given a root path collects all bootentries entries found under /bootentries/entries/.
 *
 * returns the number of entries found or a negative error value otherwise.
 */
int blspec_scan_directory(struct bootentries *bootentries, const char *root)
{
	glob_t globb;
	char *abspath;
	int ret, found = 0;
	const char *dirname = "loader/entries";
	int i;

	pr_debug("%s: %s %s\n", __func__, root, dirname);

	abspath = basprintf("%s/%s/*.conf", root, dirname);

	ret = glob(abspath, 0, NULL, &globb);
	if (ret) {
		pr_debug("%s: %s: %s\n", __func__, abspath, strerror(errno));
		ret = -errno;
		goto err_out;
	}

	for (i = 0; i < globb.gl_pathc; i++) {
		const char *configname = globb.gl_pathv[i];
		struct stat s;

		ret = stat(configname, &s);
		if (ret || !S_ISREG(s.st_mode))
			continue;

		ret = blspec_scan_file(bootentries, root, configname);
		if (ret > 0)
			found += ret;
	}

	ret = found;

	globfree(&globb);
err_out:
	free(abspath);

	return ret;
}

/*
 * blspec_scan_ubi - scan over a cdev containing UBI volumes
 *
 * This function attaches a cdev as UBI devices and collects all bootentries
 * entries found in the UBI volumes
 *
 * returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
static int blspec_scan_ubi(struct bootentries *bootentries, struct cdev *cdev)
{
	struct device *child;
	int ret, found = 0;

	pr_debug("%s: %s\n", __func__, cdev->name);

	ret = ubi_attach_mtd_dev(cdev->mtd, UBI_DEV_NUM_AUTO, 0, 20);
	if (ret && ret != -EEXIST)
		return 0;

	device_for_each_child(cdev->dev, child) {
		ret = blspec_scan_device(bootentries, child);
		if (ret > 0)
			found += ret;
	}

	return found;
}

/*
 * blspec_scan_cdev - scan over a cdev
 *
 * Given a cdev this function mounts the filesystem and collects all bootentries
 * entries found under /bootentries/entries/.
 *
 * returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
static int blspec_scan_cdev(struct bootentries *bootentries, struct cdev *cdev)
{
	int ret, found = 0;
	void *buf = xzalloc(512);
	enum filetype type, filetype;
	const char *rootpath;

	pr_debug("%s: %s\n", __func__, cdev->name);

	ret = cdev_read(cdev, buf, 512, 0, 0);
	if (ret < 0) {
		free(buf);
		return ret;
	}

	type = file_detect_partition_table(buf, 512);
	filetype = file_detect_type(buf, 512);
	free(buf);

	if (type == filetype_mbr || type == filetype_gpt)
		return -EINVAL;

	if (filetype == filetype_ubi && IS_ENABLED(CONFIG_MTD_UBI)) {
		ret = blspec_scan_ubi(bootentries, cdev);
		if (ret > 0)
			found += ret;
	}

	rootpath = cdev_mount(cdev);
	if (!IS_ERR(rootpath)) {
		ret = blspec_scan_directory(bootentries, rootpath);
		if (ret > 0)
			found += ret;
	}

	return found;
}

/*
 * blspec_scan_devices - scan all devices for child cdevs
 *
 * Iterate over all devices and collect child their cdevs.
 * Returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
int blspec_scan_devices(struct bootentries *bootentries)
{
	struct device *dev;
	struct block_device *bdev;
	int ret, found = 0;

	for_each_device(dev)
		device_detect(dev);

	for_each_block_device(bdev) {
		struct cdev *cdev;

		list_for_each_entry(cdev, &bdev->dev->cdevs, devices_list) {
			ret = blspec_scan_cdev(bootentries, cdev);
			if (ret > 0)
				found += ret;
		}
	}

	return found;
}

/*
 * blspec_scan_device - scan a device for child cdevs
 *
 * Given a device this functions scans over all child cdevs looking
 * for bootentries entries.
 * Returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
int blspec_scan_device(struct bootentries *bootentries, struct device *dev)
{
	struct device *child;
	struct cdev *cdev;
	int ret, found = 0;

	pr_debug("%s: %s\n", __func__, dev_name(dev));

	device_detect(dev);

	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		/*
		 * If the OS is installed on a disk with MBR disk label, and a
		 * partition with the MBR type id of 0xEA already exists it
		 * should be used as $BOOT
		 */
		if (cdev->dos_partition_type == 0xea) {
			ret = blspec_scan_cdev(bootentries, cdev);
			if (ret == 0)
				ret = -ENOENT;

			return ret;
		}

		/*
		 * If the OS is installed on a disk with GPT disk label, and a
		 * partition with the GPT type GUID of
		 * bc13c2ff-59e6-4262-a352-b275fd6f7172 already exists, it
		 * should be used as $BOOT.
		 *
		 * Not yet implemented
		 */
	}

	/* Try child devices */
	device_for_each_child(dev, child) {
		ret = blspec_scan_device(bootentries, child);
		if (ret > 0)
			return ret;
	}

	/*
	 * As a last resort try all cdevs (Not only the ones explicitly stated
	 * by the bootblspec spec).
	 */
	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		ret = blspec_scan_cdev(bootentries, cdev);
		if (ret > 0)
			found += ret;
	}

	return found;
}

/*
 * blspec_scan_devicename - scan a hardware device for child cdevs
 *
 * Given a name of a hardware device this functions scans over all child
 * cdevs looking for bootentries entries.
 * Returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
int blspec_scan_devicename(struct bootentries *bootentries, const char *devname)
{
	struct device *dev;
	struct cdev *cdev;

	pr_debug("%s: %s\n", __func__, devname);

	device_detect_by_name(devname);

	cdev = cdev_by_name(devname);
	if (cdev) {
		int ret = blspec_scan_cdev(bootentries, cdev);
		if (ret > 0)
			return ret;
	}

	dev = get_device_by_name(devname);
	if (!dev)
		return -ENODEV;

	return blspec_scan_device(bootentries, dev);
}

static int blspec_bootentry_provider(struct bootentries *bootentries,
				     const char *name)
{
	struct stat s;
	int ret, found = 0;

	ret = blspec_scan_devicename(bootentries, name);
	if (ret > 0)
		found += ret;

	if (*name == '/' || !strncmp(name, "nfs://", 6)) {
		char *nfspath = parse_nfs_url(name);

		if (nfspath)
			name = nfspath;

		ret = stat(name, &s);
		if (ret)
			goto out;

		if (S_ISDIR(s.st_mode))
			ret = blspec_scan_directory(bootentries, name);
		else if (S_ISREG(s.st_mode) && strends(name, ".conf"))
			ret = blspec_scan_file(bootentries, NULL, name);
		if (ret > 0)
			found += ret;

out:
		free(nfspath);
	}

	return found;
}

static int blspec_init(void)
{
	return bootentry_register_provider(blspec_bootentry_provider);
}
device_initcall(blspec_init);
