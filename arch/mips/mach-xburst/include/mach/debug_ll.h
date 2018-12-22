/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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
