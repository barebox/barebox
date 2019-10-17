/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <firmware.h>
#include <common.h>
#include <malloc.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <libbb.h>
#include <libfile.h>
#include <fs.h>
#include <linux/list.h>
#include <linux/stat.h>
#include <linux/err.h>

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
 * the device tree.
 */
struct firmware_mgr *firmwaremgr_find_by_node(const struct device_node *np)
{
	struct firmware_mgr *mgr;

	list_for_each_entry(mgr, &firmwaremgr_list, list)
		if (mgr->handler->dev->parent->device_node == np)
			return mgr;

	return NULL;
}

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

/*
 * firmware_load_file - load a firmware to a device
 */
int firmwaremgr_load_file(struct firmware_mgr *mgr, const char *firmware)
{
	int ret;
	char *name = basprintf("/dev/%s", mgr->handler->id);

	ret = copy_file(firmware, name, 0);

	free(name);

	return ret;
}
