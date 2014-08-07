#ifndef _USB_FASTBOOT_H
#define _USB_FASTBOOT_H

#include <linux/types.h>
#include <file-list.h>
#include <usb/composite.h>

struct f_fastboot_opts {
	struct usb_function_instance func_inst;
	struct file_list *files;
};

#endif /* _USB_FASTBOOT_H */
