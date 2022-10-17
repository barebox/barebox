/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_RESTART_H
#define __INCLUDE_RESTART_H

#include <linux/compiler.h>
#include <linux/types.h>

struct device_node;

void restart_handlers_print(void);
void __noreturn restart_machine(void);
struct restart_handler *restart_handler_get_by_name(const char *name);

struct device_node;

struct restart_handler {
	void (*restart)(struct restart_handler *);
	int priority;
	struct device_node *of_node;
	const char *name;
	struct list_head list;
};

int restart_handler_register(struct restart_handler *);
int restart_handler_register_fn(const char *name,
				void (*restart_fn)(struct restart_handler *));

#define RESTART_DEFAULT_PRIORITY 100

#endif /* __INCLUDE_RESTART_H */
