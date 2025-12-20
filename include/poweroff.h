/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_POWEROFF_H
#define __INCLUDE_POWEROFF_H

#include <linux/compiler.h>
#include <linux/types.h>

void __noreturn poweroff_machine(unsigned long poweroff_flags);

struct poweroff_handler {
	void (*poweroff)(struct poweroff_handler *,
			 unsigned long flags);
	int priority;
	const char *name;
	struct list_head list;
};

int poweroff_handler_register(struct poweroff_handler *);
int poweroff_handler_register_fn(void (*poweroff_fn)(struct poweroff_handler *,
						     unsigned long flags));

#define POWEROFF_DEFAULT_PRIORITY 100
#define POWEROFF_DEFAULT_NAME "default"

struct device_node;
unsigned int of_get_poweroff_priority(struct device_node *node);

#endif /* __INCLUDE_POWEROFF_H */
