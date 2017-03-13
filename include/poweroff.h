#ifndef __INCLUDE_POWEROFF_H
#define __INCLUDE_POWEROFF_H

void __noreturn poweroff_machine(void);

struct poweroff_handler {
	void (*poweroff)(struct poweroff_handler *);
	int priority;
	const char *name;
	struct list_head list;
};

int poweroff_handler_register(struct poweroff_handler *);
int poweroff_handler_register_fn(void (*poweroff_fn)(struct poweroff_handler *));

#define POWEROFF_DEFAULT_PRIORITY 100
#define POWEROFF_DEFAULT_NAME "default"

unsigned int of_get_poweroff_priority(struct device_node *node);

#endif /* __INCLUDE_POWEROFF_H */
