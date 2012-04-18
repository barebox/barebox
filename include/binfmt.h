/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#ifndef __BFMT_H__
#define __BFMT_H__

#include <filetype.h>
#include <linux/list.h>

struct binfmt_hook {
	enum filetype type;
	int (*hook)(struct binfmt_hook *b, char *file, int argc, char **argv);
	char *exec;

	struct list_head list;
};

#ifdef CONFIG_BINFMT
int binfmt_register(struct binfmt_hook *b);
void binfmt_unregister(struct binfmt_hook *b);

int execute_binfmt(int argc, char **argv);
#else
static inline int binfmt_register(struct binfmt_hook *b)
{
	return -EINVAL;
}
static inline void binfmt_unregister(struct binfmt_hook *b) {}

static inline int execute_binfmt(int argc, char **argv)
{
	return 1;
}
#endif

#endif /* __BFMT_H__ */
