/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov at gmail.com>
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

#include <common.h>
#include <io.h>
#include <mach/loongson1.h>

void __noreturn reset_cpu(ulong addr)
{
	__raw_writel(0x1, LS1X_WDT_EN);
	__raw_writel(0x1, LS1X_WDT_SET);
	__raw_writel(0x1, LS1X_WDT_TIMER);

	unreachable();
}
EXPORT_SYMBOL(reset_cpu);
