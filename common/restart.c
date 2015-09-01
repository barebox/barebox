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
#define pr_fmt(fmt) "restart: " fmt

#include <common.h>
#include <restart.h>
#include <malloc.h>
#include <of.h>

static LIST_HEAD(restart_handler_list);

/**
 * restart_handler_register() - register a handler for restarting the system
 * @rst:	The handler struct
 *
 * This adds @rst to the list of registered restart handlers.
 *
 * return: 0 for success or negative error code
 */
int restart_handler_register(struct restart_handler *rst)
{
	if (!rst->name)
		rst->name = RESTART_DEFAULT_NAME;
	if (!rst->priority)
		rst->priority = RESTART_DEFAULT_PRIORITY;

	list_add_tail(&rst->list, &restart_handler_list);

	pr_debug("registering restart handler \"%s\" with priority %d\n",
			rst->name, rst->priority);

	return 0;
}

/**
 * restart_handler_register_fn() - register a handler function
 * @restart_fn:	The restart function
 *
 * convenience wrapper for restart_handler_register() to register a handler
 * with given function and default values otherwise.
 *
 * return: 0 for success or negative error code
 */
int restart_handler_register_fn(void (*restart_fn)(struct restart_handler *))
{
	struct restart_handler *rst;
	int ret;

	rst = xzalloc(sizeof(*rst));

	rst->restart = restart_fn;

	ret = restart_handler_register(rst);

	if (ret)
		free(rst);

	return ret;
}

/**
 * restart_machine() - reset the whole system
 */
void __noreturn restart_machine(void)
{
	struct restart_handler *rst = NULL, *tmp;
	unsigned int priority = 0;

	list_for_each_entry(tmp, &restart_handler_list, list) {
		if (tmp->priority > priority) {
			priority = tmp->priority;
			rst = tmp;
		}
	}

	if (rst) {
		pr_debug("%s: using restart handler %s\n", __func__, rst->name);
		console_flush();
		rst->restart(rst);
	}

	hang();
}

/**
 * of_get_restart_priority() - get the desired restart priority from device tree
 * @node:	The device_node to read the property from
 *
 * return: The priority
 */
unsigned int of_get_restart_priority(struct device_node *node)
{
	unsigned int priority = RESTART_DEFAULT_PRIORITY;

	of_property_read_u32(node, "restart-priority", &priority);

	return priority;
}
