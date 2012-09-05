/*
 * MTD devices registration
 *
 * Copyright (C) 2011 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * mtddev_hook - hook to register additional mtd devices
 * @add_mtd_device: called when a MTD driver calls add_mtd_device()
 * @del_mtd_device: called when a MTD driver calls del_mtd_device()
 *
 * Provide a hook to be called whenether a add_mtd_device() is called.
 * Additionnal devices like mtdoob and mtdraw subscribe to the service.
 */
struct mtddev_hook {
	struct list_head hook;
	int (*add_mtd_device)(struct mtd_info *mtd, char *devname, void **priv);
	int (*del_mtd_device)(struct mtd_info *mtd, void **priv);
	void *priv;
};
struct cdev;

/**
 * mtdcore_add_hook - add a hook to MTD registration/unregistration
 * @hook: the hook
 *
 * Normally called in a coredevice_initcall() to add another MTD layout (such as
 * mtdraw, ...)
 */
void mtdcore_add_hook(struct mtddev_hook *hook);

int mtd_ioctl(struct cdev *cdev, int request, void *buf);
