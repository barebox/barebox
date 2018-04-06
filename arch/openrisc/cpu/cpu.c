/*
 * (C) Copyright 2011, Stefan Kristiansson <stefan.kristiansson@saunalahti.fi>
 * (C) Copyright 2011, Julius Baxter <julius@opencores.org>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <restart.h>
#include <asm/system.h>
#include <asm/openrisc_exc.h>

extern void __reset(void);

static void __noreturn openrisc_restart_cpu(struct restart_handler *rst)
{
	__reset();
	/* not reached, __reset does not return */

	/* Not reached */
	hang();
}

static int restart_register_feature(void)
{
	return restart_handler_register_fn(openrisc_restart_cpu);
}
coredevice_initcall(restart_register_feature);
