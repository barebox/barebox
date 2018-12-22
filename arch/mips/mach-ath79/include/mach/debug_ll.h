/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 */

#ifndef __MACH_ATH79_DEBUG_LL__
#define __MACH_ATH79_DEBUG_LL__

#ifdef CONFIG_DEBUG_LL

#ifdef CONFIG_DEBUG_AR9331_UART
#include <mach/debug_ll_ar9331.h>
#elif defined CONFIG_DEBUG_AR9344_UART
#include <mach/debug_ll_ar9344.h>
#else
#error "unknown ath79 debug uart soc type"
#endif

#else
#define debug_ll_ath79_init
#endif  /* CONFIG_DEBUG_LL */

#endif /* __MACH_AR9344_DEBUG_LL_H__ */
