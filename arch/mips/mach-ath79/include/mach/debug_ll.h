/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __MACH_ATH79_DEBUG_LL__
#define __MACH_ATH79_DEBUG_LL__

#if defined(CONFIG_SOC_QCA_AR9331)
#include <mach/debug_ll_ar9331.h>
#elif defined(CONFIG_SOC_QCA_AR9344)
#include <mach/debug_ll_ar9344.h>
#endif

#endif /* __MACH_AR9344_DEBUG_LL_H__ */
