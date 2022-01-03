/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __USB_GADGET_MULTI_H
#define __USB_GADGET_MULTI_H

#include <usb/fastboot.h>
#include <usb/dfu.h>
#include <usb/usbserial.h>
#include <usb/mass_storage.h>

struct f_multi_opts {
	struct fastboot_opts fastboot_opts;
	struct f_dfu_opts dfu_opts;
	struct f_ums_opts ums_opts;
	bool create_acm;
	void (*release)(struct f_multi_opts *opts);
};

int usb_multi_register(struct f_multi_opts *opts);
void usb_multi_unregister(void);
void usb_multi_opts_release(struct f_multi_opts *opts);
unsigned usb_multi_count_functions(struct f_multi_opts *opts);

#define USBGADGET_EXPORT_BBU	(1 << 0)
#define USBGADGET_ACM		(1 << 1)
#define USBGADGET_DFU		(1 << 2)
#define USBGADGET_FASTBOOT	(1 << 3)
#define USBGADGET_MASS_STORAGE	(1 << 4)

struct usbgadget_funcs {
	int flags;
	const char *fastboot_opts;
	const char *dfu_opts;
	const char *ums_opts;
};

int usbgadget_register(const struct usbgadget_funcs *funcs);

#endif /* __USB_GADGET_MULTI_H */
