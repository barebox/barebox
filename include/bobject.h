/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BOBJECT_H_
#define __BOBJECT_H_

#include <linux/list.h>
#include <linux/compiler.h>

/**
 * struct bobject - barebox object
 * @name: name of object (must be first member)
 * @parameters: list of struct param_d parameters
 */
struct bobject {
	char			*name;
	struct list_head	parameters;
};

struct device;

typedef union {
	struct bobject *bobj;
	struct device *dev;
} bobject_t __attribute__((transparent_union));


__printf(2, 3) int bobject_set_name(bobject_t bobj, const char *name, ...);

static inline void bobject_init(struct bobject *bobj)
{
	/* We have code that iterates over parameters unconditionally,
	 * so initialize it even if !IS_ENABLED(CONFIG_PARAMETER)
	 */
	INIT_LIST_HEAD(&bobj->parameters);
}

struct bobject *bobject_alloc(const char *name);
void bobject_free(struct bobject *bobj);

#ifdef CONFIG_PARAMETER
void bobject_del(struct bobject *bobj);
#else
static inline void bobject_del(struct bobject *bobj) { }
#endif

#endif
