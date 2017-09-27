#ifndef __INCLUDE_BBU_H
#define __INCLUDE_BBU_H

#include <asm-generic/errno.h>
#include <linux/list.h>
#include <linux/types.h>
#include <filetype.h>

struct bbu_data {
#define BBU_FLAG_FORCE	(1 << 0)
#define BBU_FLAG_YES	(1 << 1)
	unsigned long flags;
	int force;
	void *image;
	const char *imagefile;
	const char *devicefile;
	size_t len;
	const char *handler_name;
	struct imd_header *imd_data;
};

struct bbu_handler {
	int (*handler)(struct bbu_handler *, struct bbu_data *);
	const char *name;
	struct list_head list;
#define BBU_HANDLER_FLAG_DEFAULT	(1 << 0)
#define BBU_HANDLER_CAN_REFRESH		(1 << 1)
	unsigned long flags;

	/* default device file, can be overwritten on the command line */
	const char *devicefile;
};

int bbu_force(struct bbu_data *, const char *fmt, ...)
	__attribute__ ((format(__printf__, 2, 3)));

int bbu_confirm(struct bbu_data *);

int barebox_update(struct bbu_data *);

bool barebox_update_handler_exists(struct bbu_data *);

void bbu_handlers_list(void);

int bbu_handlers_iterate(int (*fn)(struct bbu_handler *, void *), void *);

#ifdef CONFIG_BAREBOX_UPDATE

int bbu_register_handler(struct bbu_handler *);

int bbu_register_std_file_update(const char *name, unsigned long flags,
		char *devicefile, enum filetype imagetype);

#else

static inline int bbu_register_handler(struct bbu_handler *unused)
{
	return -EINVAL;
}

static inline int bbu_register_std_file_update(const char *name, unsigned long flags,
		char *devicefile, enum filetype imagetype)
{
	return -ENOSYS;
}
#endif

#if defined(CONFIG_BAREBOX_UPDATE_IMX_NAND_FCB)
int imx6_bbu_nand_register_handler(const char *name, unsigned long flags);
int imx28_bbu_nand_register_handler(const char *name, unsigned long flags);
#else
static inline int imx6_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	return -ENOSYS;
}
static inline int imx28_bbu_nand_register_handler(const char *name, unsigned long flags)
{
	return -ENOSYS;
}
#endif

#endif /* __INCLUDE_BBU_H */
