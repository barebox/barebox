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
};

static enum reset_src_type reset_source;

enum reset_src_type reset_source_get(void)
{
	return reset_source;
}
EXPORT_SYMBOL(reset_source_get);

void reset_source_set(enum reset_src_type st)
{
	reset_source = st;

	globalvar_add_simple("system.reset", reset_src_names[reset_source]);
}
EXPORT_SYMBOL(reset_source_set);

/* ensure this runs after the 'global' device is already registerd */
static int reset_source_init(void)
{
	reset_source_set(reset_source);

	return 0;
}

coredevice_initcall(reset_source_init);
