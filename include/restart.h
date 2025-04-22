/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_RESTART_H
#define __INCLUDE_RESTART_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/bitops.h>

struct device_node;

void restart_handlers_print(void);
void __noreturn restart_machine(unsigned long restart_flags);
struct restart_handler *restart_handler_get_by_name(const char *name, int flags);

struct device_node;

struct restart_handler {
	void (*restart)(struct restart_handler *, unsigned long);
	int priority;
#define RESTART_FLAG_WARM_BOOTROM	BIT(0)
	int flags;
	struct device_node *of_node;
	const char *name;
	struct device *dev;
	struct list_head list;
};

int restart_handler_register(struct restart_handler *);
int restart_handler_register_fn(const char *name,
				void (*restart_fn)(struct restart_handler *,
						   unsigned long));

#define RESTART_DEFAULT_PRIORITY 100

#endif /* __INCLUDE_RESTART_H */
