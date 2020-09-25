/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_RESTART_H
#define __INCLUDE_RESTART_H

void restart_handlers_print(void);
void __noreturn restart_machine(void);
struct restart_handler *restart_handler_get_by_name(const char *name);

struct restart_handler {
	void (*restart)(struct restart_handler *);
	int priority;
	const char *name;
	struct list_head list;
};

int restart_handler_register(struct restart_handler *);
int restart_handler_register_fn(const char *name,
				void (*restart_fn)(struct restart_handler *));

#define RESTART_DEFAULT_PRIORITY 100

unsigned int of_get_restart_priority(struct device_node *node);

#endif /* __INCLUDE_RESTART_H */
