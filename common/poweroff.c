/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt) "poweroff: " fmt

#include <common.h>
#include <poweroff.h>
#include <malloc.h>
#include <of.h>

static LIST_HEAD(poweroff_handler_list);

/**
 * poweroff_handler_register() - register a handler for poweroffing the system
 * @rst:	The handler struct
 *
 * This adds @rst to the list of registered poweroff handlers.
 *
 * return: 0 for success or negative error code
 */
int poweroff_handler_register(struct poweroff_handler *handler)
{
	if (!handler->name)
		handler->name = POWEROFF_DEFAULT_NAME;
	if (!handler->priority)
		handler->priority = POWEROFF_DEFAULT_PRIORITY;

	list_add_tail(&handler->list, &poweroff_handler_list);

	pr_debug("registering poweroff handler \"%s\" with priority %d\n",
			handler->name, handler->priority);

	return 0;
}

/**
 * poweroff_handler_register_fn() - register a handler function
 * @poweroff_fn:	The poweroff function
 *
 * convenience wrapper for poweroff_handler_register() to register a handler
 * with given function and default values otherwise.
 *
 * return: 0 for success or negative error code
 */
int poweroff_handler_register_fn(void (*poweroff_fn)(struct poweroff_handler *))
{
	struct poweroff_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));

	handler->poweroff = poweroff_fn;

	ret = poweroff_handler_register(handler);

	if (ret)
		free(handler);

	return ret;
}

/**
 * poweroff_machine() - power off the machine
 */
void __noreturn poweroff_machine(void)
{
	struct poweroff_handler *handler = NULL, *tmp;
	unsigned int priority = 0;

	list_for_each_entry(tmp, &poweroff_handler_list, list) {
		if (tmp->priority > priority) {
			priority = tmp->priority;
			handler = tmp;
		}
	}

	if (handler) {
		pr_debug("using poweroff handler %s\n", handler->name);
		console_flush();
		handler->poweroff(handler);
	} else {
		pr_err("No poweroff handler found!\n");
	}

	hang();
}

/**
 * of_get_poweroff_priority() - get the desired poweroff priority from device tree
 * @node:	The device_node to read the property from
 *
 * return: The priority
 */
unsigned int of_get_poweroff_priority(struct device_node *node)
{
	unsigned int priority = POWEROFF_DEFAULT_PRIORITY;

	of_property_read_u32(node, "poweroff-priority", &priority);

	return priority;
}
