// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2012 Juergen Beisert - <kernel@pengutronix.de>
 */
#define pr_fmt(fmt) "reset-source: " fmt

#include <common.h>
#include <init.h>
#include <environment.h>
#include <globalvar.h>
#include <reset_source.h>
#include <magicvar.h>

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
static struct device *reset_source_device;

enum reset_src_type reset_source_get(void)
{
	return reset_source;
}
EXPORT_SYMBOL(reset_source_get);

const char *reset_source_to_string(enum reset_src_type st)
{
	return reset_src_names[st];
}
EXPORT_SYMBOL(reset_source_to_string);

int reset_source_get_instance(void)
{
	return reset_source_instance;
}
EXPORT_SYMBOL(reset_source_get_instance);

struct device *reset_source_get_device(void)
{
	return reset_source_device;
}
EXPORT_SYMBOL(reset_source_get_device);

static void __reset_source_set(struct device *dev,
			       enum reset_src_type st,
			       unsigned int priority, int instance)
{
	if (priority <= reset_source_priority)
		return;

	reset_source = st;
	reset_source_priority = priority;
	reset_source_instance = instance;
	reset_source_device = dev;

	pr_debug("Setting reset source to %s with priority %d\n",
			reset_src_names[reset_source], priority);
}

void reset_source_set_prinst(enum reset_src_type st,
			     unsigned int priority, int instance)
{
	__reset_source_set(NULL, st, priority, instance);
}
EXPORT_SYMBOL(reset_source_set_prinst);

void reset_source_set_device(struct device *dev, enum reset_src_type st,
			     unsigned int priority)
{
	of_property_read_u32(dev->of_node, "reset-source-priority", &priority);

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

BAREBOX_MAGICVAR(global.system.reset, "The reason for the last system reset")
BAREBOX_MAGICVAR(global.system.reset_instance, "The reset reason instance number")
