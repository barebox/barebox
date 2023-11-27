/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_BBU_H
#define __INCLUDE_BBU_H

#include <linux/errno.h>
#include <linux/list.h>
#include <linux/types.h>
#include <filetype.h>

struct bbu_data {
#define BBU_FLAG_FORCE	(1 << 0)
#define BBU_FLAG_YES	(1 << 1)
#define BBU_FLAG_MMC_BOOT_ACK	(1 << 2)
	unsigned long flags;
	int force;
	const void *image;
	const char *imagefile;
	const char *devicefile;
	size_t len;
	const char *handler_name;
	const struct imd_header *imd_data;
};

struct bbu_handler {
	int (*handler)(struct bbu_handler *, struct bbu_data *);
	const char *name;
	struct list_head list;
#define BBU_HANDLER_FLAG_DEFAULT	(1 << 0)
#define BBU_HANDLER_CAN_REFRESH		(1 << 1)
	/*
	 * The lower 16bit are generic flags, the upper 16bit are reserved
	 * for handler specific flags.
	 */
	unsigned long flags;

	/* default device file, can be overwritten on the command line */
	const char *devicefile;
};

int bbu_force(struct bbu_data *, const char *fmt, ...)
	__attribute__ ((format(__printf__, 2, 3)));

int bbu_confirm(struct bbu_data *);

int barebox_update(struct bbu_data *, struct bbu_handler *);

struct bbu_handler *bbu_find_handler_by_name(const char *name);
struct bbu_handler *bbu_find_handler_by_device(const char *devicepath);

void bbu_handlers_list(void);

struct file_list;

int bbu_mmcboot_handler(struct bbu_handler *, struct bbu_data *,
			int (*chained_handler)(struct bbu_handler *, struct bbu_data *));

int bbu_std_file_handler(struct bbu_handler *handler,
			 struct bbu_data *data);

#ifdef CONFIG_BAREBOX_UPDATE

int bbu_register_handler(struct bbu_handler *);

int bbu_register_std_file_update(const char *name, unsigned long flags,
		const char *devicefile, enum filetype imagetype);

void bbu_append_handlers_to_file_list(struct file_list *files);

#else

static inline int bbu_register_handler(struct bbu_handler *unused)
{
	return -EINVAL;
}

static inline int bbu_register_std_file_update(const char *name, unsigned long flags,
		const char *devicefile, enum filetype imagetype)
{
	return -ENOSYS;
}

static inline void bbu_append_handlers_to_file_list(struct file_list *files)
{
	/* none could be registered, so nothing to do */
}

#endif

#if defined(CONFIG_BAREBOX_UPDATE_IMX_NAND_FCB)
int imx6_bbu_nand_register_handler(const char *name, unsigned long flags);
int imx7_bbu_nand_register_handler(const char *name, unsigned long flags);
int imx28_bbu_nand_register_handler(const char *name, unsigned long flags);
#else
static inline int imx6_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	return -ENOSYS;
}
static inline int imx7_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	return -ENOSYS;
}
static inline int imx28_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	return -ENOSYS;
}
#endif

#endif /* __INCLUDE_BBU_H */
