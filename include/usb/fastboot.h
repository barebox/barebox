#ifndef _USB_FASTBOOT_H
#define _USB_FASTBOOT_H

#include <linux/types.h>
#include <file-list.h>
#include <usb/composite.h>

struct f_fastboot;

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
	int (*cmd_exec)(struct f_fastboot *, const char *cmd);
	int (*cmd_flash)(struct f_fastboot *, struct file_list_entry *entry,
			 const char *filename, const void *buf, size_t len);
};

/*
 * Return codes for the exec_cmd callback above:
 *
 * FASTBOOT_CMD_FALLTHROUGH - Not handled by the external command dispatcher,
 *                            handle it with internal dispatcher
 * Other than these negative error codes mean errors handling the command and
 * zero means the command has been successfully handled.
 */
#define FASTBOOT_CMD_FALLTHROUGH	1

enum fastboot_msg_type {
	FASTBOOT_MSG_OKAY,
	FASTBOOT_MSG_FAIL,
	FASTBOOT_MSG_INFO,
	FASTBOOT_MSG_DATA,
};

int fastboot_tx_print(struct f_fastboot *f_fb, enum fastboot_msg_type type,
		      const char *fmt, ...);

#endif /* _USB_FASTBOOT_H */
