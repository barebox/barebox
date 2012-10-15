#ifndef __INCLUDE_BBU_H
#define __INCLUDE_BBU_H

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
};

struct bbu_handler {
	int (*handler)(struct bbu_handler *, struct bbu_data *);
	const char *name;
	struct list_head list;
#define BBU_HANDLER_FLAG_DEFAULT	(1 << 0)
	unsigned long flags;

	/* default device file, can be overwritten on the command line */
	const char *devicefile;
};

int bbu_force(struct bbu_data *, const char *fmt, ...)
	__attribute__ ((format(__printf__, 2, 3)));

int bbu_confirm(struct bbu_data *);

int barebox_update(struct bbu_data *);

void bbu_handlers_list(void);

#ifdef CONFIG_BAREBOX_UPDATE

int bbu_register_handler(struct bbu_handler *);

#else

static inline int bbu_register_handler(struct bbu_handler *unused)
{
	return -EINVAL;
}

#endif

#endif /* __INCLUDE_BBU_H */
