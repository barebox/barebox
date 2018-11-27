/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __MACH_XBURST_DEBUG_LL__
#define __MACH_XBURST_DEBUG_LL__

/** @file
 *  This File contains declaration for early output support
 */

#ifdef CONFIG_DEBUG_LL

#ifdef CONFIG_DEBUG_JZ4750D_UART
#include <mach/debug_ll_jz4750d.h>
#elif defined CONFIG_DEBUG_JZ4780_UART
#include <mach/debug_ll_jz4780.h>
#else
#error "unknown xburst debug uart soc type"
#endif

#endif  /* CONFIG_DEBUG_LL */

#include <asm/debug_ll_ns16550.h>

#endif  /* __MACH_XBURST_DEBUG_LL__ */
