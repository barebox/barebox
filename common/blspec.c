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
#include <readkey.h>
#include <common.h>
#include <driver.h>
#include <blspec.h>
#include <malloc.h>
#include <block.h>
#include <fcntl.h>
#include <libbb.h>
#include <init.h>
#include <boot.h>
#include <fs.h>
#include <of.h>
#include <linux/stat.h>
#include <linux/err.h>

/*
 * blspec_entry_var_set - set a variable to a value
 */
int blspec_entry_var_set(struct blspec_entry *entry, const char *name,
		const char *val)
{
	return of_set_property(entry->node, name, val,
			val ? strlen(val) + 1 : 0, 1);
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

/*
 * blspec_entry_open - open an entry given a path
 */
static struct blspec_entry *blspec_entry_open(struct blspec *blspec,
		const char *abspath)
{
	struct blspec_entry *entry;
	char *end, *line, *next;
	char *buf;

	pr_debug("%s: %s\n", __func__, abspath);

	buf = read_file(abspath, NULL);
	if (!buf)
		return ERR_PTR(-errno);

	entry = blspec_entry_alloc(blspec);

	next = buf;

	while (*next) {
		char *name, *val;

		line = next;

		next = strchr(line, '\n');
		if (next) {
			*next = 0;
			next++;
		}

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

		blspec_entry_var_set(entry, name, val);
	}

	free(buf);

	return entry;
}

/*
 * blspec_have_entry - check if we already have an entry with
 *                     a certain path
 */
static int blspec_have_entry(struct blspec *blspec, const char *path)
{
	struct blspec_entry *e;

	list_for_each_entry(e, &blspec->entries, list) {
		if (e->configpath && !strcmp(e->configpath, path))
			return 1;
	}

	return 0;
}

/*
 * blspec_scan_directory - scan over a directory
 *
 * Given a root path collects all blspec entries found under /blspec/entries/.
 *
 * returns 0 if at least one entry could be successfully loaded, negative
 * error value otherwise.
 */
static int blspec_scan_directory(struct blspec *blspec, const char *root,
		struct cdev *cdev)
{
	struct blspec_entry *entry;
	DIR *dir;
	struct dirent *d;
	char *abspath;
	int ret, found = 0;
	const char *dirname = "loader/entries";
	char *entry_default = NULL, *entry_once = NULL, *name;

	pr_debug("%s: %s %s\n", __func__, root, dirname);

	entry_default = read_file_line("%s/default", root);
	entry_once = read_file_line("%s/once", root);

	abspath = asprintf("%s/%s", root, dirname);

	dir = opendir(abspath);
	if (!dir) {
		pr_debug("%s: %s: %s\n", __func__, abspath, strerror(errno));
		ret = -errno;
		goto err_out;
	}

	while ((d = readdir(dir))) {
		char *configname;
		struct stat s;
		char *dot;
		char *devname = NULL, *hwdevname = NULL;

		if (*d->d_name == '.')
			continue;

		configname = asprintf("%s/%s", abspath, d->d_name);

		dot = strrchr(configname, '.');
		if (!dot) {
			free(configname);
			continue;
		}

		if (strcmp(dot, ".conf")) {
			free(configname);
			continue;
		}

		ret = stat(configname, &s);
		if (ret) {
			free(configname);
			continue;
		}

		if (!S_ISREG(s.st_mode)) {
			free(configname);
			continue;
		}

		if (blspec_have_entry(blspec, configname)) {
			free(configname);
			continue;
		}

		entry = blspec_entry_open(blspec, configname);
		if (IS_ERR(entry)) {
			free(configname);
			continue;
		}

		found = 1;

		entry->rootpath = xstrdup(root);
		entry->configpath = configname;
		entry->cdev = cdev;

		name = asprintf("%s/%s", dirname, d->d_name);
		if (entry_default && !strcmp(name, entry_default))
			entry->boot_default = true;
		if (entry_once && !strcmp(name, entry_once))
			entry->boot_once = true;
		free(name);

		devname = xstrdup(dev_name(entry->cdev->dev));
		if (entry->cdev->dev->parent)
			hwdevname = xstrdup(dev_name(entry->cdev->dev->parent));

		entry->me.display = asprintf("%-20s %-20s  %s", devname, hwdevname,
				blspec_entry_var_get(entry, "title"));
		free(devname);
		free(hwdevname);

		entry->me.type = MENU_ENTRY_NORMAL;
	}

	ret = found ? 0 : -ENOENT;

	closedir(dir);
err_out:
	free(abspath);
	free(entry_default);
	free(entry_once);

	return ret;
}

/*
 * blspec_scan_cdev - scan over a cdev
 *
 * Given a cdev this function mounts the filesystem and collects all blspec
 * entries found under /blspec/entries/.
 *
 * returns 0 if at least one entry could be successfully loaded, negative
 * error value otherwise.
 */
static int blspec_scan_cdev(struct blspec *blspec, struct cdev *cdev)
{
	int ret;
	void *buf = xzalloc(512);
	enum filetype type;
	const char *rootpath;

	pr_debug("%s: %s\n", __func__, cdev->name);

	ret = cdev_read(cdev, buf, 512, 0, 0);
	if (ret < 0) {
		free(buf);
		return ret;
	}

	type = file_detect_partition_table(buf, 512);
	free(buf);

	if (type == filetype_mbr || type == filetype_gpt)
		return -EINVAL;

	rootpath = cdev_mount_default(cdev);
	if (IS_ERR(rootpath))
		return PTR_ERR(rootpath);

	ret = blspec_scan_directory(blspec, rootpath, cdev);

	return ret;
}

/*
 * blspec_scan_devices - scan all devices for child cdevs
 *
 * Iterate over all devices and collect child their cdevs.
 */
void blspec_scan_devices(struct blspec *blspec)
{
	struct device_d *dev;
	struct block_device *bdev;

	for_each_device(dev)
		device_detect(dev);

	for_each_block_device(bdev) {
		struct cdev *cdev = &bdev->cdev;

		list_for_each_entry(cdev, &bdev->dev->cdevs, devices_list)
			blspec_scan_cdev(blspec, cdev);
	}
}

/*
 * blspec_scan_device - scan a device for child cdevs
 *
 * Given a device this functions scans over all child cdevs looking
 * for blspec entries.
 */
int blspec_scan_device(struct blspec *blspec, struct device_d *dev)
{
	struct device_d *child;
	struct cdev *cdev;
	int ret;

	pr_debug("%s: %s\n", __func__, dev_name(dev));

	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		/*
		 * If the OS is installed on a disk with MBR disk label, and a
		 * partition with the MBR type id of 0xEA already exists it
		 * should be used as $BOOT
		 */
		if (cdev->dos_partition_type == 0xea) {
			blspec_scan_cdev(blspec, cdev);
			return 0;
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
		ret = blspec_scan_device(blspec, child);
		if (!ret)
			return 0;
	}

	/*
	 * As a last resort try all cdevs (Not only the ones explicitly stated
	 * by the bootblspec spec).
	 */
	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		ret = blspec_scan_cdev(blspec, cdev);
		if (!ret)
			return 0;
	}

	return -ENODEV;
}

/*
 * blspec_scan_hwdevice - scan a hardware device for child cdevs
 *
 * Given a name of a hardware device this functions scans over all child
 * cdevs looking for blspec entries.
 */
int blspec_scan_hwdevice(struct blspec *blspec, const char *devname)
{
	struct device_d *dev;

	pr_debug("%s: %s\n", __func__, devname);

	dev = get_device_by_name(devname);
	if (!dev)
		return -ENODEV;

	device_detect(dev);

	blspec_scan_device(blspec, dev);

	return 0;
}

/*
 * blspec_boot - boot an entry
 *
 * This boots an entry. On success this function does not return.
 * In case of an error the error code is returned. This function may
 * return 0 in case of a succesful dry run.
 */
int blspec_boot(struct blspec_entry *entry, int verbose, int dryrun)
{
	int ret;
	const char *abspath, *devicetree, *options, *initrd, *linuximage;
	struct bootm_data data = {
		.initrd_address = UIMAGE_INVALID_ADDRESS,
		.os_address = UIMAGE_SOME_ADDRESS,
		.verbose = verbose,
		.dryrun = dryrun,
	};

	globalvar_set_match("linux.bootargs.dyn.", "");
	globalvar_set_match("bootm.", "");

	devicetree = blspec_entry_var_get(entry, "devicetree");
	initrd = blspec_entry_var_get(entry, "initrd");
	options = blspec_entry_var_get(entry, "options");
	linuximage = blspec_entry_var_get(entry, "linux");

	if (entry->rootpath)
		abspath = entry->rootpath;
	else
		abspath = "";

	data.os_file = asprintf("%s/%s", abspath, linuximage);

	if (devicetree) {
		if (!strcmp(devicetree, "none")) {
			struct device_node *node = of_get_root_node();
			if (node)
				of_delete_node(node);
		} else {
			data.oftree_file = asprintf("%s/%s", abspath,
					devicetree);
		}
	}

	if (initrd)
		data.initrd_file = asprintf("%s/%s", abspath, initrd);

	globalvar_add_simple("linux.bootargs.blspec", options);

	pr_info("booting %s from %s\n", blspec_entry_var_get(entry, "title"),
			dev_name(entry->cdev->dev));

	if (entry->boot_once) {
		char *s = asprintf("%s/once", abspath);

		ret = unlink(s);
		if (ret)
			pr_err("unable to unlink 'once': %s\n", strerror(-ret));
		else
			pr_info("removed 'once'\n");

		free(s);
	}

	ret = bootm_boot(&data);
	if (ret)
		pr_err("Booting failed\n");

	free((char *)data.oftree_file);
	free((char *)data.initrd_file);
	free((char *)data.os_file);

	return ret;
}

/*
 * blspec_entry_default - find the entry to load.
 *
 * return in the order of precendence:
 * - The entry specified in the 'once' file
 * - The entry specified in the 'default' file
 * - The first entry
 */
struct blspec_entry *blspec_entry_default(struct blspec *l)
{
	struct blspec_entry *entry_once = NULL;
	struct blspec_entry *entry_default = NULL;
	struct blspec_entry *entry_first = NULL;
	struct blspec_entry *e;

	list_for_each_entry(e, &l->entries, list) {
		if (!entry_first)
			entry_first = e;
		if (e->boot_once)
			entry_once = e;
		if (e->boot_default)
			entry_default = e;
	}

	if (entry_once)
		return entry_once;
	if (entry_default)
		return entry_default;
	return entry_first;
}

/*
 * blspec_boot_hwdevice - scan hardware device for blspec entries and
 *                        start the best one.
 */
int blspec_boot_hwdevice(const char *devname, int verbose, int dryrun)
{
	struct blspec *blspec;
	struct blspec_entry *e;
	int ret;

	blspec = blspec_alloc();

	ret = blspec_scan_hwdevice(blspec, devname);
	if (ret)
		return ret;

	e = blspec_entry_default(blspec);
	if (!e) {
		printf("Nothing found on %s\n", devname);
		ret = -ENOENT;
		goto out;
	}

	ret = blspec_boot(e, verbose, dryrun);
out:
	blspec_free(blspec);

	return ret;
}
