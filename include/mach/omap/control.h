/**
 * @file
 * @brief This file contains the Control register defines
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

#ifndef __ASM_ARCH_OMAP_CONTROL_H
#define __ASM_ARCH_OMAP_CONTROL_H

/**
 * Control register defintion which unwraps to the real register
 * offset + base address
 */
#define OMAP3_CONTROL_REG(REGNAME)	(OMAP3_CTRL_BASE + CONTROL_##REGNAME)

#define CONTROL_SCALABLE_OMAP_STATUS    (0x44C)
#define CONTROL_SCALABLE_OMAP_OCP       (0x534)
#define CONTROL_SCRATCHPAD_BASE		(0x910)
#define CONTROL_SCRATCHPAD_ROM_BASE	(0x860)
#define CONTROL_STATUS			(0x2f0)
#define CONTROL_SYSCONFIG               (0x010)
#define CONTROL_DEVCONF0		(0x274)
#define CONTROL_DEVCONF1		(0x2D8)
#define CONTROL_IVA2_BOOTMOD		(0x404)
#define CONTROL_IVA2_BOOTADDR		(0x400)
#define CONTROL_PBIAS_1			(0x520)
#define CONTROL_GENERAL_PURPOSE_STATUS	(0x2F4)
#define CONTROL_MEM_DFTRW0		(0x278)
#define CONTROL_MEM_DFTRW1		(0x27C)
#define CONTROL_MSUSPENDMUX_0		(0x290)
#define CONTROL_MSUSPENDMUX_1		(0x294)
#define CONTROL_MSUSPENDMUX_2		(0x298)
#define CONTROL_MSUSPENDMUX_3		(0x29C)
#define CONTROL_MSUSPENDMUX_4		(0x2A0)
#define CONTROL_MSUSPENDMUX_5		(0x2A4)
#define CONTROL_SEC_CTRL		(0x2B0)
#define CONTROL_CSIRXFE			(0x2DC)
#define CONTROL_DEBOBS_0		(0x420)
#define CONTROL_DEBOBS_1		(0x424)
#define CONTROL_DEBOBS_2		(0x428)
#define CONTROL_DEBOBS_3		(0x42C)
#define CONTROL_DEBOBS_4		(0x430)
#define CONTROL_DEBOBS_5		(0x434)
#define CONTROL_DEBOBS_6		(0x438)
#define CONTROL_DEBOBS_7		(0x43C)
#define CONTROL_DEBOBS_8		(0x440)
#define CONTROL_PROG_IO0		(0x444)
#define CONTROL_PROG_IO1		(0x448)
#define CONTROL_DSS_DPLL_SPREADING	(0x450)
#define CONTROL_CORE_DPLL_SPREADING	(0x454)
#define CONTROL_PER_DPLL_SPREADING	(0x458)
#define CONTROL_USBHOST_DPLL_SPREADING	(0x45C)
#define CONTROL_TEMP_SENSOR		(0x524)
#define CONTROL_SRAMLDO4		(0x528)
#define CONTROL_SRAMLDO5		(0x52C)
#define CONTROL_CSI			(0x530)
#define CONTROL_SCALABLE_OMAP_OCP	(0x534)
#define CONTROL_SCALABLE_OMAP_STATUS	(0x44C)

/** Provide the Regoffset, Value */
#define	MUX_VAL(OFFSET,VALUE)\
	writew((VALUE), OMAP3_CTRL_BASE + (OFFSET))

/**
 * macro for Padconfig Registers @see
 * include/mach-arm/arch-omap/omap3-mux.h
 */
#define	CP(X) (CONTROL_PADCONF_##X)

#endif /* __ASM_ARCH_OMAP_CONTROL_H */
