// SPDX-License-Identifier: GPL-2.0-or-later

#define pr_fmt(fmt)  "efi-loader: bootesp: " fmt

#include <malloc.h>
#include <fcntl.h>
#include <libfile.h>
#include <libbb.h>
#include <init.h>
#include <bootm.h>
#include <driver.h>
#include <fs.h>
#include <globalvar.h>
#include <linux/stat.h>
#include <linux/list.h>
#include <linux/err.h>
#include <spec/dps.h>
#include <boot.h>
#include <memory.h>
#include <firmware.h>
#include <command.h>

#include <bootscan.h>

struct esp_entry {
	struct bootentry entry;

	struct cdev *cdev;
	const char *exepath;
	const char *rootpath;
};

/*
 * esp_boot - boot an entry
 *
 * This boots an entry. On success this function does not return.
 * In case of an error the error code is returned. This function may
 * return 0 in case of a successful dry run.
 */
static int esp_boot(struct bootentry *be, int verbose, int dryrun)
{
	struct esp_entry *entry = container_of(be, struct esp_entry, entry);
	struct bootm_data data = {};
	int ret;

	bootm_data_init_defaults(&data);
	data.verbose = max(verbose, data.verbose);
	data.dryrun = dryrun;
	data.efi_boot = BOOTM_EFI_REQUIRED;

	data.os_file = strdup_const(entry->exepath);

	/* TODO:
	 * 1) move firmware/overlay handling into common/bootm.c and then
	 * 2) implement device tree overlay patching protocol using it?
	 */

	ret = bootm_boot(&data);
	if (ret)
		pr_err("Booting failed\n");

	free_const(data.os_file);

	return ret;
}

static void esp_entry_free(struct bootentry *be)
{
	struct esp_entry *entry = container_of(be, struct esp_entry, entry);

	free_const(entry->exepath);
	free_const(entry->rootpath);
	free(entry);
}

static struct esp_entry *esp_entry_alloc(struct bootentries *bootentries)
{
	struct esp_entry *entry;

	entry = xzalloc(sizeof(*entry));

	entry->entry.release = esp_entry_free;
	entry->entry.boot = esp_boot;

	return entry;
}

static int __esp_scan_file(struct bootentries *bootentries, const char *root,
			   const char *exename)
{
	char *devname = NULL, *hwdevname = NULL;
	struct esp_entry *entry;

	pr_debug("%s: %s\n", __func__, root);

	entry = esp_entry_alloc(bootentries);
	if (IS_ERR(entry))
		return PTR_ERR(entry);

	root = root ?: get_mounted_path(exename);
	entry->rootpath = xstrdup_const(root);
	entry->exepath = xstrdup_const(exename);
	entry->cdev = get_cdev_by_mountpath(root);

	if (entry->cdev && entry->cdev->dev) {
		devname = xstrdup(dev_name(entry->cdev->dev));
		if (entry->cdev->dev->parent)
			hwdevname = xstrdup(dev_name(entry->cdev->dev->parent));
	}

	entry->entry.title = xasprintf("EFI payload (%s)", exename);
	entry->entry.description = basprintf("ESP entry, device: %s hwdevice: %s",
					    devname ? devname : "none",
					    hwdevname ? hwdevname : "none");
	free(devname);
	free(hwdevname);

	entry->entry.me.type = MENU_ENTRY_NORMAL;
	entry->entry.release = esp_entry_free;

	bootentries_add_entry(bootentries, &entry->entry);
	return 1;
}

static int esp_scan_file(struct bootscanner *scanner,
			    struct bootentries *bootentries,
			    const char *exename)
{
	u8 magic[2];

	if (read_file_into_buf(exename, &magic, 2) != 2 ||!is_dos_exe(magic))
		return 0;

	return __esp_scan_file(bootentries, NULL, exename);
}

/*
 * esp_scan_directory - scan over a directory
 *
 * Given a root path collects all bootentries.
 *
 * returns the number of entries found or a negative error value otherwise.
 */
static int esp_scan_directory(struct bootscanner *bootscanner,
				  struct bootentries *bootentries,
				  const char *root)
{
	const char *path = CONFIG_EFI_PAYLOAD_DEFAULT_PATH;
	char *abspath;
	int fd, ret, found = 0;
	struct stat s;

	pr_debug("%s: %s\n", __func__, root);

	fd = open(root, O_DIRECTORY);
	if (fd < 0)
		return fd;

	ret = fixup_path_case(fd, &path);
	if (ret < 0)
		goto out;
	fd = ret;

	ret = fstat(fd, &s);
	if (ret || !S_ISREG(s.st_mode))
		goto out;

	abspath = xasprintf("%s/%s", root, path);

	ret = __esp_scan_file(bootentries, root, abspath);
	if (ret > 0)
		found += ret;

	free(abspath);

	ret = found;
out:
	close(fd);
	return ret;
}

static int esp_scan_with_flag(struct bootscanner *scanner,
			      struct bootentries *bootentries, struct cdev *cdev,
			      unsigned flags)
{
	struct cdev *partcdev;
	int found = 0, err = 0;
	int ret;

	for_each_cdev_partition(partcdev, cdev) {
		if ((partcdev->flags & flags) != flags)
			continue;

		err = -ENOENT;

		if (partcdev->typeflags & DPS_TYPE_FLAG_NO_AUTO) {
			pr_debug("%s: partition skipped from autodiscovery\n",
				 partcdev->name);
			continue;
		}

		ret = boot_scan_cdev(scanner, bootentries, partcdev, true);
		if (ret > 0)
			found += ret;
	}

	return found ?: err;
}

/*
 * esp_scan_disk - scan a disk cdev for ESP
 *
 * Given a cdev this functions scans over all child partitions looking
 * for the ESP.
 * Returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
static int esp_scan_disk(struct bootscanner *scanner,
			 struct bootentries *bootentries, struct cdev *cdev)
{
	int nesp = 0, nlegacy = 0;

	pr_debug("%s: %s\n", __func__, cdev->name);

	nesp = esp_scan_with_flag(scanner, bootentries, cdev,
				  DEVFS_PARTITION_BOOTABLE_ESP);
	if (nesp < 0)
		return nesp;

	nlegacy = esp_scan_with_flag(scanner, bootentries, cdev,
				     DEVFS_PARTITION_BOOTABLE_LEGACY);
	if (nlegacy < 0)
		return nlegacy;

	return nesp + nlegacy;
}

static struct bootscanner esp_scanner = {
	.name		= "esp",
	.scan_file	= esp_scan_file,
	.scan_directory	= esp_scan_directory,
	.scan_disk	= esp_scan_disk,
};

static int esp_bootentry_generate(struct bootentries *bootentries, const char *name)
{
	return bootentry_scan_generate(&esp_scanner, bootentries, name);
}

static struct bootentry_provider esp_bootentry_provider = {
	.name = "esp",
	.generate = esp_bootentry_generate,
	.priority = -50,
};

static int esp_init(void)
{
	return bootentry_register_provider(&esp_bootentry_provider);
}
device_initcall(esp_init);
