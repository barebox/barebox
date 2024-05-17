/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * MTD devices registration
 *
 * Copyright (C) 2011 Robert Jarzmik
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
	int (*add_mtd_device)(struct mtd_info *mtd, const char *devname, void **priv);
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

int mtd_ioctl(struct cdev *cdev, unsigned int request, void *buf);
