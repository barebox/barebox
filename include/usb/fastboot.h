#ifndef _USB_FASTBOOT_H
#define _USB_FASTBOOT_H

#include <linux/types.h>
#include <file-list.h>
#include <usb/composite.h>

/**
 * struct f_fastboot_opts - options to configure the fastboot gadget
 * @func_inst:	The USB function instance to register on
 * @files:	A file_list containing the files (partitions) to export via fastboot
 * @export_bbu:	Automatically include the partitions provided by barebox update (bbu)
 */
struct f_fastboot_opts {
	struct usb_function_instance func_inst;
	struct file_list *files;
	bool export_bbu;
};

#endif /* _USB_FASTBOOT_H */
