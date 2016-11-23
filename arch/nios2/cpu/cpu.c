/*
 * (C) Copyright 2004, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
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

static void __noreturn nios2_restart_soc(struct restart_handler *rst)
{
	/* indirect call to go beyond 256MB limitation of toolchain */
	nios2_callr(RESET_ADDR);

	/* Not reached */
	hang();
}

static int restart_register_feature(void)
{
	return restart_handler_register_fn(nios2_restart_soc);
}
coredevice_initcall(restart_register_feature);

