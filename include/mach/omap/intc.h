/**
 * @file
 * @brief This file contains the Interrupt controller register defines
 *
 * Originally from Linux kernel:
 * http://linux.omap.com/pub/kernel/3430zoom/linux-ldp-v1.0b.tar.gz
 * include/asm-arm/arch-omap/omap34xx.h
 *
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * Copyright (C) 2007 Texas Instruments, <www.ti.com>
 * Copyright (C) 2007 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_OMAP_INTC_H
#define __ASM_ARCH_OMAP_INTC_H

/** Interrupt Controller Register wrapper */
#define INTC_REG(REGNAME)	(OMAP_INTC_BASE + INTC_##REGNAME)

#define INTC_MIR_0		(0x084)
#define INTC_MIR_1		(0x0A4)
#define INTC_MIR_2		(0x0C4)
#define INTC_MIR_SET_0		(0x08C)
#define INTC_MIR_SET_1		(0x0AC)
#define INTC_MIR_SET_2		(0x0CC)
#define INTC_MIR_CLEAR_0	(0x094)
#define INTC_MIR_CLEAR_1	(0x0B4)
#define INTC_MIR_CLEAR_2	(0x0D4)
#define INTC_PS_SYSCONFIG       (0x010)
#define INTC_PS_PROTECTION      (0x04C)
#define INTC_PS_IDLE            (0x050)
#define INTC_PS_THRESHOLD       (0x068)
#define INTC_PS_PENDING_IRQ0	(0x098)
#define INTC_PS_PENDING_IRQ1	(0x0B8)
#define INTC_PS_PENDING_IRQ2	(0x0D8)

#endif /* __ASM_ARCH_OMAP_INTC_H */
