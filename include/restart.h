#ifndef __INCLUDE_RESTART_H
#define __INCLUDE_RESTART_H

void __noreturn restart_machine(void);

struct restart_handler {
	void (*restart)(struct restart_handler *);
	int priority;
	const char *name;
	struct list_head list;
};

int restart_handler_register(struct restart_handler *);
int restart_handler_register_fn(void (*restart_fn)(struct restart_handler *));

#define RESTART_DEFAULT_PRIORITY 100
#define RESTART_DEFAULT_NAME "default"

unsigned int of_get_restart_priority(struct device_node *node);

#endif /* __INCLUDE_RESTART_H */
