/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2011 Samsung Electrnoics
 * Lukasz Majewski <l.majewski@samsung.com>
 */

#ifndef __USB_MASS_STORAGE_H__
#define __USB_MASS_STORAGE_H__

#include <usb/composite.h>

/* Wait at maximum 60 seconds for cable connection */
#define UMS_CABLE_READY_TIMEOUT	60

struct fsg_common;

struct f_ums_opts {
	struct usb_function_instance func_inst;
	struct fsg_common *common;
	struct file_list *files;
	unsigned int num_sectors;
	int fd;
	int refcnt;
	char name[16];
};

int usb_ums_register(struct f_ums_opts *);

#endif /* __USB_MASS_STORAGE_H__ */
