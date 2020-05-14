#ifndef _USB_FASTBOOT_H
#define _USB_FASTBOOT_H

#include <usb/composite.h>
#include <fastboot.h>

/**
 * struct f_fastboot_opts - options to configure the fastboot gadget
 * @common:	Options common to all fastboot protocol variants
 * @func_inst:	The USB function instance to register on
 */
struct f_fastboot_opts {
	struct fastboot_opts common;
	struct usb_function_instance func_inst;
};

#endif /* _USB_FASTBOOT_H */
