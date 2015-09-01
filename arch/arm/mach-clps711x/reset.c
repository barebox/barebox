/*
 * Copyright (C) 2012 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <restart.h>

static void __noreturn clps711x_restart_soc(struct restart_handler *rst)
{
	shutdown_barebox();

	asm("mov pc, #0");

	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(clps711x_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
