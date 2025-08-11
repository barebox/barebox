/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BOBJECT_H_
#define __BOBJECT_H_

#include <linux/compiler.h>

/**
 * struct bobject - barebox object
 * @name: name of object (must be first member)
 */
struct bobject {
	char			*name;
};

struct device;

typedef union {
	struct bobject *bobj;
	struct device *dev;
} bobject_t __attribute__((transparent_union));


__printf(2, 3) int bobject_set_name(bobject_t bobj, const char *name, ...);

#endif
