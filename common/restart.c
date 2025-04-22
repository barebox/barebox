// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#define pr_fmt(fmt) "restart: " fmt

#include <common.h>
#include <restart.h>
#include <malloc.h>
#include <of.h>

static LIST_HEAD(restart_handler_list);
static unsigned resetidx;

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
		rst->name = basprintf("reset%u", resetidx);
	if (!rst->priority)
		rst->priority = RESTART_DEFAULT_PRIORITY;

	if (rst->of_node) {
		of_property_read_u32(rst->of_node, "restart-priority",
				     &rst->priority);
		if (of_property_read_bool(rst->of_node,
					  "barebox,restart-warm-bootrom"))
			rst->flags |= RESTART_FLAG_WARM_BOOTROM;
	}

	list_add_tail(&rst->list, &restart_handler_list);

	pr_debug("registering restart handler \"%s\" with priority %d\n",
			rst->name, rst->priority);

	resetidx++;
	return 0;
}

/**
 * restart_handler_register_fn() - register a handler function
 * @name:	restart method name or NULL if name should be auto-generated
 * @restart_fn:	The restart function
 *
 * convenience wrapper for restart_handler_register() to register a handler
 * with given function and default values otherwise.
 *
 * return: 0 for success or negative error code
 */
int restart_handler_register_fn(const char *name,
				void (*restart_fn)(struct restart_handler *))
{
	struct restart_handler *rst;
	int ret;

	rst = xzalloc(sizeof(*rst));

	rst->name = xstrdup_const(name);
	rst->restart = restart_fn;

	ret = restart_handler_register(rst);

	if (ret)
		free(rst);

	return ret;
}

/**
 * restart_handler_get_by_name() - get highest priority `name'
 */
struct restart_handler *restart_handler_get_by_name(const char *name, int flags)
{
	struct restart_handler *rst = NULL, *tmp;
	unsigned int priority = 0;

	list_for_each_entry(tmp, &restart_handler_list, list) {
		if (name) {
			if ((tmp->name && strcmp(name, tmp->name)) &&
			    (tmp->dev && strcmp(name, dev_name(tmp->dev))))
				continue;
		}

		if (flags && (tmp->flags & flags) != flags)
			continue;
		if (tmp->priority > priority) {
			priority = tmp->priority;
			rst = tmp;
		}
	}

	return rst;
}

/**
 * restart_machine() - reset the whole system
 */
void __noreturn restart_machine(unsigned long flags)
{
	struct restart_handler *rst;

	rst = restart_handler_get_by_name(NULL, 0);
	if (rst) {
		pr_debug("%s: using restart handler %s\n", __func__, rst->name);
		console_flush();
		rst->restart(rst);
	}

	hang();
}

/*
 * restart_handlers_print - print informations about all restart handlers
 */
void restart_handlers_print(void)
{
	struct restart_handler *tmp;

	list_for_each_entry(tmp, &restart_handler_list, list) {
		printf("%-20s %-20s %6d ",
		       tmp->name, tmp->dev ? dev_name(tmp->dev) : "", tmp->priority);
		if (tmp->flags & RESTART_FLAG_WARM_BOOTROM)
			putchar('W');
		putchar('\n');
	}
}
