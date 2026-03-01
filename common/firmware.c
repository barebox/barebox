// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 */

#include <firmware.h>
#include <common.h>
#include <malloc.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <libbb.h>
#include <libfile.h>
#include <fs.h>
#include <globalvar.h>
#include <magicvar.h>
#include <linux/list.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <uncompress.h>
#include <filetype.h>

#define BUFSIZ 4096

struct firmware_mgr {
	struct list_head list;
	struct firmware_handler *handler; /* the program handler */
	struct cdev cdev;
	u8 buf[BUFSIZ];
	int ofs;
};

static LIST_HEAD(firmwaremgr_list);

/*
 * firmwaremgr_find - find a firmware device handler
 *
 * Find a firmware device handler based on the unique id. If @id is
 * NULL this returns the single firmware device handler if only one
 * is registered. If multiple handlers are registered @id is mandatory
 *
 */
struct firmware_mgr *firmwaremgr_find(const char *id)
{
	struct firmware_mgr *mgr;

	if (!id) {
		if (list_is_singular(&firmwaremgr_list))
			return list_first_entry(&firmwaremgr_list,
					struct firmware_mgr, list);
		else
			return NULL;
	}

	list_for_each_entry(mgr, &firmwaremgr_list, list)
		if (!strcmp(mgr->handler->id, id))
			return mgr;

	return NULL;
}

/*
 * firmwaremgr_find_by_node - find a firmware device handler
 *
 * Find a firmware device handler using the device node of the firmware
 * handler. This allows to retrieve the firmware handler with a phandle from
 * the device tree. If np is NULL, returns the first instance encountered.
 */
struct firmware_mgr *firmwaremgr_find_by_node(struct device_node *np)
{
	struct firmware_mgr *mgr;
	char *na;

	na = of_get_reproducible_name(np);

	list_for_each_entry(mgr, &firmwaremgr_list, list) {
		char *nb = of_get_reproducible_name(mgr->handler->device_node);
		if (!na || !strcmp(na, nb)) {
			free(na);
			free(nb);
			return mgr;
		}

		free(nb);
	}

	free(na);

	return NULL;
}
EXPORT_SYMBOL(firmwaremgr_find_by_node);

/*
 * firmwaremgr_list_handlers - list registered firmware device handlers
 *                             in pretty format
 */
void firmwaremgr_list_handlers(void)
{
	struct firmware_mgr *mgr;

	printf("firmware programming handlers:\n\n");

	if (list_empty(&firmwaremgr_list)) {
		printf("(none)\n");
		return;
	}

	printf("%-11s%-11s\n", "name:", "model:");

	list_for_each_entry(mgr, &firmwaremgr_list, list) {
		printf("%-11s", mgr->handler->id);
		if (mgr->handler->model)
			printf(" -> %-11s", mgr->handler->model);
		printf("\n");
	}
}

static int firmware_open(struct cdev *cdev, unsigned long flags)
{
	struct firmware_mgr *mgr = cdev->priv;
	int ret;

	mgr->ofs = 0;

	ret = mgr->handler->open(mgr->handler);
	if (ret)
		return ret;

	return 0;
}

static ssize_t firmware_write(struct cdev *cdev, const void *buf, size_t insize,
		loff_t offset, ulong flags)
{
	struct firmware_mgr *mgr = cdev->priv;
	int ret;
	size_t count = insize;

	/*
	 * We guarantee the write handler of the firmware device that only the
	 * last write is a short write. All others are 4k in size.
	 */

	while (count) {
		size_t space = BUFSIZ - mgr->ofs;
		size_t now = min(count, space);

		memcpy(mgr->buf + mgr->ofs, buf, now);

		buf += now;
		mgr->ofs += now;
		count -= now;

		if (mgr->ofs == BUFSIZ) {
			ret = mgr->handler->write(mgr->handler, mgr->buf, BUFSIZ);
			if (ret < 0)
				return ret;

			mgr->ofs = 0;
		}
	}

	return insize;
}

static int firmware_close(struct cdev *cdev)
{
	struct firmware_mgr *mgr = cdev->priv;
	int ret;

	if (mgr->ofs) {
		ret = mgr->handler->write(mgr->handler, mgr->buf, mgr->ofs);
		if (ret)
			return ret;
	}

	ret = mgr->handler->close(mgr->handler);
	if (ret)
		return ret;

	return 0;
}

static struct cdev_operations firmware_ops = {
	.open = firmware_open,
	.write = firmware_write,
	.close = firmware_close,
};

/*
 * firmwaremgr_register - register a device which needs firmware
 */
int firmwaremgr_register(struct firmware_handler *fh)
{
	struct firmware_mgr *mgr;
	int ret;
	struct cdev *cdev;

	if (firmwaremgr_find(fh->id))
		return -EBUSY;

	mgr = xzalloc(sizeof(struct firmware_mgr));
	mgr->handler = fh;

	cdev = &mgr->cdev;

	cdev->name = xstrdup(fh->id);
	cdev->flags = DEVFS_IS_CHARACTER_DEV;
	cdev->ops = &firmware_ops;
	cdev->priv = mgr;
	cdev->dev = fh->dev;

	ret = devfs_create(cdev);
	if (ret)
		goto out;

	list_add_tail(&mgr->list, &firmwaremgr_list);

	return 0;
out:
	free(cdev->name);
	free(mgr);

	return ret;
}

static char *firmware_path;

char *firmware_get_searchpath(void)
{
	return strdup(firmware_path);
}

void firmware_set_searchpath(const char *path)
{
	free(firmware_path);
	firmware_path = strdup(path);
}

static bool file_exists(const char *filename)
{
	struct stat s;

	return !stat(filename, &s);
}

/*
 * firmware_load_file - load a firmware to a device
 */
int firmwaremgr_load_file(struct firmware_mgr *mgr, const char *firmware)
{
	char *dst;
	enum filetype type;
	int ret = 0;
	char *fw = NULL;
	int firmwarefd = 0;
	int devicefd = 0;

	if (!firmware)
		return -EINVAL;

	if (!mgr->handler->id) {
		pr_err("id not defined for handler\n");
		return -ENODEV;
	}

	dst = basprintf("/dev/%s", mgr->handler->id);

	if (*firmware != '/') {
		fw = find_path(firmware_path, firmware, file_exists);
		if (fw)
			firmware = fw;
	}

	firmwarefd = open(firmware, O_RDONLY);
	if (firmwarefd < 0) {
		printf("could not open %s: %m\n", firmware);
		ret = firmwarefd;
		goto out;
	}

	ret = file_name_detect_type(firmware, &type);
	if (ret)
		goto out;

	devicefd = open(dst, O_WRONLY);
	if (devicefd < 0) {
		printf("could not open %s: %m\n", dst);
		ret = devicefd;
		goto out;
	}

	if (file_is_compressed_file(type))
		ret = uncompress_fd_to_fd(firmwarefd, devicefd,
					  uncompress_err_stdout);
	else
		ret = copy_fd(firmwarefd, devicefd, 0);

out:
	free(dst);
	free(fw);

	if (firmwarefd > 0)
		close(firmwarefd);
	if (devicefd > 0)
		close(devicefd);

	return ret;
}

/*
 * request_firmware - load a firmware to a device
 */
int request_firmware(const struct firmware **out, const char *fw_name, struct device *dev)
{
	char fw_path[PATH_MAX + 1];
	struct firmware *fw;
	int ret;

	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);
	if (!fw)
		return -ENOMEM;

	snprintf(fw_path, sizeof(fw_path), "%s/%s", firmware_path, fw_name);

	ret = read_file_2(fw_path, &fw->size, (void *)&fw->data, FILESIZE_MAX);
	if (ret) {
		kfree(fw);
		return ret;
	}

	*out = fw;

	return 0;
}

void release_firmware(const struct firmware *fw)
{
	kfree_const(fw->data);
	kfree_const(fw);
}

static int firmware_init(void)
{
	firmware_path = strdup("/env/firmware");
	globalvar_add_simple_string("firmware.path", &firmware_path);

	return 0;
}
device_initcall(firmware_init);

BAREBOX_MAGICVAR(global.firmware.path, "Firmware search path");
