/*
 * barebox - interrupt.S Processing of interrupts and exception handling
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This file is based on interrupt.S
 *
 * Copyright (C) 2003  Metrowerks, Inc. <mwaddel@metrowerks.com>
 * Copyright (C) 2002  Arcturus Networks Ltd. Ted Ma <mated@sympatico.ca>
 * Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 *                     The Silver Hammer Group, Ltd.
 *
 * (c) 1995, Dionne & Associates
 * (c) 1995, DKG Display Tech.
 *
 * This file is also based on exception.asm
 * (C) Copyright 2001-2005 - Analog Devices, Inc.  All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 *
 */

#define ASSEMBLY
#include <config.h>
#include <asm/blackfin.h>
#include <asm/hw_irq.h>
#include <asm/entry.h>
#include <asm/blackfin_defs.h>
#include <asm/irq.h>

.global _blackfin_irq_panic;

.text
.align 2

#ifndef CONFIG_KGDB
.global _evt_emulation
_evt_emulation:
	SAVE_CONTEXT
	r0 = IRQ_EMU;
	r1 = seqstat;
	sp += -12;
	call _blackfin_irq_panic;
	sp += 12;
	rte;
#endif

.global _evt_nmi
_evt_nmi:
	SAVE_CONTEXT
	r0 = IRQ_NMI;
	r1 = RETN;
	sp += -12;
	call _blackfin_irq_panic;
	sp += 12;

_evt_nmi_exit:
	rtn;

.global _trap
_trap:
	SAVE_ALL_SYS
        r0 = sp;        /* stack frame pt_regs pointer argument ==> r0 */
        sp += -12;
        call _trap_c
        sp += 12;
        RESTORE_ALL_SYS
        rtx;

.global _evt_ivhw
_evt_ivhw:
	SAVE_CONTEXT
	RAISE 14;

_evt_ivhw_exit:
	 rti;

