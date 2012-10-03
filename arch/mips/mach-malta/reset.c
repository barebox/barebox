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
#include <mach/hardware.h>

void __noreturn reset_cpu(ulong addr)
{
	__raw_writel(GORESET, (char *)SOFTRES_REG);
	while (1);
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);
