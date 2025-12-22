// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#define pr_fmt(fmt) "poweroff: " fmt

#include <common.h>
#include <poweroff.h>
#include <malloc.h>
#include <efi/types.h>
#include <asm/sections.h>
#include <of.h>

static LIST_HEAD(poweroff_handler_list);

static __efi_runtime_data void (*rt_poweroff)(unsigned long flags);
static int rt_poweroff_prio = INT_MIN;

static void rt_poweroff_handler_register(struct poweroff_handler *handler)
{
	if (!IS_ENABLED(CONFIG_EFI_RUNTIME))
		return;
	if (!handler->rt_poweroff)
		return;
	if (!in_barebox_efi_runtime((ulong)handler->rt_poweroff)) {
		/* Check if __efi_runtime attribute is missing */
		pr_warn("handler outside EFI runtime section\n");
		return;
	}
	if (handler->priority <= rt_poweroff_prio)
		return;

	rt_poweroff = handler->rt_poweroff;
	rt_poweroff_prio = handler->priority;
}

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

	rt_poweroff_handler_register(handler);

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
int poweroff_handler_register_fn(void (*poweroff_fn)(struct poweroff_handler *,
						     unsigned long flags))
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
 * rt_poweroff_machine() - power off the machine from a runtime service
 */
void __noreturn __efi_runtime rt_poweroff_machine(unsigned long flags)
{
	if (rt_poweroff)
		rt_poweroff(flags);

	__hang();
}

/**
 * poweroff_machine() - power off the machine
 */
void __noreturn poweroff_machine(unsigned long flags)
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
		handler->poweroff(handler, flags);
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
