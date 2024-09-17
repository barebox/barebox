/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __FASTBOOT__
#define __FASTBOOT__

#include <common.h>
#include <file-list.h>
#include <net.h>

#define FASTBOOT_MAX_CMD_LEN  64

/*
 * Return codes for the exec_cmd callback above:
 *
 * FASTBOOT_CMD_FALLTHROUGH - Not handled by the external command dispatcher,
 *                            handle it with internal dispatcher
 * Other than these negative error codes mean errors handling the command and
 * zero means the command has been successfully handled.
 */
#define FASTBOOT_CMD_FALLTHROUGH	1

struct fastboot {
	int (*write)(struct fastboot *fb, const char *buf, unsigned int n);
	void (*start_download)(struct fastboot *fb);

	struct file_list *files;
	int (*cmd_exec)(struct fastboot *fb, const char *cmd);
	int (*cmd_flash)(struct fastboot *fb, struct file_list_entry *entry,
			 const char *filename, size_t len);
	int download_fd;
	char *tempname;

	bool active;

	size_t download_bytes;
	size_t download_size;
	struct list_head variables;
};

/**
 * struct fastboot_opts - options to configure fastboot
 * @files:	A file_list containing the files (partitions) to export via fastboot
 * @export_bbu:	Automatically include the partitions provided by barebox update (bbu)
 */
struct fastboot_opts {
	struct file_list *files;
	bool export_bbu;
	int (*cmd_exec)(struct fastboot *fb, const char *cmd);
	int (*cmd_flash)(struct fastboot *fb, struct file_list_entry *entry,
			 const char *filename, size_t len);
};

enum fastboot_msg_type {
	FASTBOOT_MSG_OKAY,
	FASTBOOT_MSG_FAIL,
	FASTBOOT_MSG_INFO,
	FASTBOOT_MSG_DATA,
	FASTBOOT_MSG_NONE,
};

#ifdef CONFIG_FASTBOOT_BASE
bool get_fastboot_bbu(void);
void set_fastboot_bbu(unsigned int enable);
struct file_list *get_fastboot_partitions(void);
#else
static inline int get_fastboot_bbu(void)
{
	return false;
}

static inline void set_fastboot_bbu(unsigned int enable)
{
}

static inline struct file_list *get_fastboot_partitions(void)
{
	return file_list_parse("");
}
#endif

int fastboot_generic_init(struct fastboot *fb, bool export_bbu);
void fastboot_generic_close(struct fastboot *fb);
void fastboot_generic_free(struct fastboot *fb);
int fastboot_handle_download_data(struct fastboot *fb, const void *buffer,
				  unsigned int len);
int fastboot_tx_print(struct fastboot *fb, enum fastboot_msg_type type,
		      const char *fmt, ...);
void fastboot_start_download_generic(struct fastboot *fb);
void fastboot_download_finished(struct fastboot *fb);
void fastboot_exec_cmd(struct fastboot *fb, const char *cmdbuf)
        __attribute__((nonnull));
void fastboot_abort(struct fastboot *fb);

#endif
