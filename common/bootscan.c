// SPDX-License-Identifier: GPL-2.0-or-later
#define pr_fmt(fmt)  "bootscan: " fmt

#include <driver.h>
#include <xfuncs.h>
#include <block.h>
#include <fs.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <mtd/ubi-user.h>

#include <bootscan.h>

static int boot_scan_device(struct bootscanner *scanner,
			    struct bootentries *bootentries, struct device *dev)
{
	struct cdev *cdev;
	int ret;

	pr_debug("%s(%s): %s\n", __func__, scanner->name, dev_name(dev));

	device_detect(dev);

	if (!scanner->scan_disk)
		goto scan_device;

	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		if (cdev_is_partition(cdev))
			continue;

		ret = scanner->scan_disk(scanner, bootentries, cdev);
		if (ret)
			return ret;
	}

scan_device:
	return scanner->scan_device(scanner, bootentries, dev);
}

/*
 * boot_scan_ubi - scan over a cdev containing UBI volumes
 *
 * This function attaches a cdev as UBI devices and collects all bootentries
 * entries found in the UBI volumes
 *
 * returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
static int boot_scan_ubi(struct bootscanner *scanner,
			 struct bootentries *bootentries, struct cdev *cdev)
{
	struct device *child;
	int ret, found = 0;

	pr_debug("%s(%s): %s\n", __func__, scanner->name, cdev->name);

	if (!scanner->scan_disk && !scanner->scan_device)
		return 0;

	ret = ubi_attach_mtd_dev(cdev->mtd, UBI_DEV_NUM_AUTO, 0, 20);
	if (ret && ret != -EEXIST)
		return 0;

	device_for_each_child(cdev->dev, child) {
		ret = boot_scan_device(scanner, bootentries, child);
		if (ret > 0)
			found += ret;
	}

	return found;
}

/*
 * boot_scan_cdev - scan over a cdev
 *
 * Given a cdev this function mounts the filesystem and collects all bootentries
 * entries found in it.
 *
 * returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
int boot_scan_cdev(struct bootscanner *scanner,
		   struct bootentries *bootentries, struct cdev *cdev)
{
	int ret, found = 0;
	void *buf = xzalloc(512);
	enum filetype type, filetype;
	const char *rootpath;

	pr_debug("%s(%s): %s\n", __func__, scanner->name, cdev->name);

	ret = cdev_read(cdev, buf, 512, 0, 0);
	if (ret < 0) {
		free(buf);
		return ret;
	}

	type = file_detect_partition_table(buf, 512);
	filetype = file_detect_type(buf, 512);
	free(buf);

	if (type == filetype_mbr || type == filetype_gpt) {
		if (!scanner->scan_disk || cdev_is_partition(cdev))
			return -EINVAL;
		return scanner->scan_disk(scanner, bootentries, cdev);
	}

	if (filetype == filetype_ubi && IS_ENABLED(CONFIG_MTD_UBI)) {
		ret = boot_scan_ubi(scanner, bootentries, cdev);
		if (ret > 0)
			found += ret;
	}

	rootpath = cdev_mount(cdev);
	if (!IS_ERR(rootpath) && scanner->scan_directory) {
		ret = scanner->scan_directory(scanner, bootentries, rootpath);
		if (ret > 0)
			found += ret;
	}

	return found;
}

/*
 * boot_scan_devicename - scan a hardware device for child cdevs
 *
 * Given a name of a hardware device this functions scans over all child
 * cdevs looking for bootentries entries.
 * Returns the number of entries found or a negative error code if some unexpected
 * error occurred.
 */
static int boot_scan_devicename(struct bootscanner *scanner,
				struct bootentries *bootentries, const char *devname)
{
	struct device *dev;
	struct cdev *cdev;

	pr_debug("%s(%s): %s\n", __func__, scanner->name, devname);

	/* Support both boot /dev/disk0.rootfs and boot disk0.rootfs */
	devname += str_has_prefix(devname, "/dev/");

	device_detect_by_name(devname);

	cdev = cdev_by_name(devname);
	if (cdev) {
		int ret = boot_scan_cdev(scanner, bootentries, cdev);
		if (ret > 0)
			return ret;
	}

	if (!scanner->scan_device)
		return 0;

	dev = get_device_by_name(devname);
	if (!dev)
		return -ENODEV;

	return boot_scan_device(scanner, bootentries, dev);
}

int bootentry_scan_generate(struct bootscanner *scanner,
			    struct bootentries *bootentries,
			    const char *name)
{
	struct stat s;
	int ret, found = 0;

	ret = boot_scan_devicename(scanner, bootentries, name);
	if (ret > 0)
		found += ret;

	if (*name == '/') {
		ret = stat(name, &s);
		if (ret)
			return found;

		if (S_ISDIR(s.st_mode) && scanner->scan_directory)
			ret = scanner->scan_directory(scanner, bootentries, name);
		else if (S_ISREG(s.st_mode) && scanner->scan_file)
			ret = scanner->scan_file(scanner, bootentries, name);
		if (ret > 0)
			found += ret;
	}

	return found;
}
