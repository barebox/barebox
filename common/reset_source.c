/*
 * (C) Copyright 2012 Juergen Beisert - <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "reset-source: " fmt

#include <common.h>
#include <init.h>
#include <environment.h>
#include <globalvar.h>
#include <reset_source.h>

static const char * const reset_src_names[] = {
	[RESET_UKWN] = "unknown",
	[RESET_POR] = "POR",
	[RESET_RST] = "RST",
	[RESET_WDG] = "WDG",
	[RESET_WKE] = "WKE",
	[RESET_JTAG] = "JTAG",
	[RESET_THERM] = "THERM",
	[RESET_EXT] = "EXT",
	[RESET_BROWNOUT] = "BROWNOUT",
};

static enum reset_src_type reset_source;
static unsigned int reset_source_priority;
static int reset_source_instance;
static struct device_d *reset_source_device;

enum reset_src_type reset_source_get(void)
{
	return reset_source;
}
EXPORT_SYMBOL(reset_source_get);

const char *reset_source_name(void)
{
	return reset_src_names[reset_source];
}
EXPORT_SYMBOL(reset_source_name);

int reset_source_get_instance(void)
{
	return reset_source_instance;
}
EXPORT_SYMBOL(reset_source_get_instance);

struct device_d *reset_source_get_device(void)
{
	return reset_source_device;
}
EXPORT_SYMBOL(reset_source_get_device);

static void __reset_source_set(struct device_d *dev,
			       enum reset_src_type st,
			       unsigned int priority, int instance)
{
	if (priority <= reset_source_priority)
		return;

	reset_source = st;
	reset_source_priority = priority;
	reset_source_instance = instance;
	reset_source_device = NULL;

	pr_debug("Setting reset source to %s with priority %d\n",
			reset_src_names[reset_source], priority);
}

void reset_source_set_prinst(enum reset_src_type st,
			     unsigned int priority, int instance)
{
	__reset_source_set(NULL, st, priority, instance);
}
EXPORT_SYMBOL(reset_source_set_prinst);

void reset_source_set_device(struct device_d *dev, enum reset_src_type st)
{
	int priority = of_get_reset_source_priority(dev->device_node);

	__reset_source_set(dev, st, priority, -1);
}
EXPORT_SYMBOL(reset_source_set_device);

static int reset_source_init(void)
{
	globalvar_add_simple_enum("system.reset", (unsigned int *)&reset_source,
			reset_src_names, ARRAY_SIZE(reset_src_names));

	globalvar_add_simple_int("system.reset_instance", &reset_source_instance,
				 "%d");
	return 0;
}
coredevice_initcall(reset_source_init);

/**
 * of_get_reset_source_priority() - get the desired reset source priority from
 *                                  device tree
 * @node:	The device_node to read the property from
 *
 * return: The priority
 */
unsigned int of_get_reset_source_priority(struct device_node *node)
{
	unsigned int priority = RESET_SOURCE_DEFAULT_PRIORITY;

	of_property_read_u32(node, "reset-source-priority", &priority);

	return priority;
}
