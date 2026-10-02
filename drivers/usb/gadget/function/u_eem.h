/* SPDX-License-Identifier: GPL-2.0 */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/usb/gadget/function/u_eem.h?id=4ccdccff8febc5456aff684627f9a4c5c83b9346 */
/*
 * u_eem.h
 *
 * Utility definitions for the eem function
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrzej Pietrasiewicz <andrzejtp2010@gmail.com>
 */

#ifndef U_EEM_H
#define U_EEM_H

#include <linux/usb/composite.h>

struct eth_device;

struct f_eem_opts {
	struct usb_function_instance	func_inst;
	struct eth_device		*net;
	bool				bound;
};

#endif /* U_EEM_H */
