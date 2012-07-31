/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ASM_CONFIG_H_
#define _ASM_CONFIG_H_

#ifdef CONFIG_MPC85xx
#include <mach/config_mpc85xx.h>
#endif

#ifndef MAX_MEM_MAPPED
#if defined(CONFIG_E500)
#define MAX_MEM_MAPPED	((phys_size_t)(2 << 30))
#endif
#endif

/*
 * Provide a default boot page translation virtual address that lines up with
 * Freescale's default e500 reset page.
 */
#if (defined(CONFIG_E500) && defined(CONFIG_MP))
#ifndef BPTR_VIRT_ADDR
#define BPTR_VIRT_ADDR	0xfffff000
#endif
#endif

#endif /* _ASM_CONFIG_H_ */
