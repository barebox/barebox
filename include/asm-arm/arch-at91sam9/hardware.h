/*
 * [origin: Linux kernel include/asm-arm/arch-at91/hardware.h]
 *
 *  Copyright (C) 2003 SAN People
 *  Copyright (C) 2003 ATMEL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#if defined(CONFIG_ARCH_AT91SAM9260)
#include <asm/arch/AT91SAM9260_inc.h>
#elif defined(CONFIG_ARCH_AT91SAM9261)
#include <asm/arch/AT91SAM9261_inc.h>
#elif defined(CONFIG_ARCH_AT91SAM9263)
#include <asm/arch/AT91SAM9263_inc.h>
#elif defined(CONFIG_ARCH_AT91SAM9RL)
#include <asm/arch/AT91SAM9RL_inc.h>
#elif defined(CONFIG_ARCH_AT91CAP9)
#include <asm/arch/AT91CAP9_inc.h>
#else
#error "Unsupported AT91 processor"
#endif

#endif
