/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
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

/**
 * @file
 * @brief Resetting an malta board
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>
#include <mach/hardware.h>

static void __noreturn malta_restart_soc(struct restart_handler *rst)
{
	__raw_writel(GORESET, (char *)SOFTRES_REG);

	hang();
	/*NOTREACHED*/
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(malta_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
