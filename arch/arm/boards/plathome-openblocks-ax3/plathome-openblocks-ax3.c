/*
 * Copyright (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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
 *
 */

#include <common.h>
#include <init.h>
#include <mach/mvebu.h>

static int plathome_openblocks_ax3_console_init(void)
{
	return mvebu_add_uart0();
}

console_initcall(plathome_openblocks_ax3_console_init);
