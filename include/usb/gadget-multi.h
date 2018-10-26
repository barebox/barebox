#ifndef __USB_GADGET_MULTI_H
#define __USB_GADGET_MULTI_H

#include <usb/fastboot.h>
#include <usb/dfu.h>
#include <usb/usbserial.h>

struct f_multi_opts {
	struct f_fastboot_opts fastboot_opts;
	struct f_dfu_opts dfu_opts;
	int create_acm;
	void (*release)(struct f_multi_opts *opts);
};

int usb_multi_register(struct f_multi_opts *opts);
void usb_multi_unregister(void);
void usb_multi_opts_release(struct f_multi_opts *opts);

int usbgadget_register(bool dfu, const char *dfu_opts,
		       bool fastboot, const char *fastboot_opts,
		       bool acm, bool export_bbu);

#endif /* __USB_GADGET_MULTI_H */
