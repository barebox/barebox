/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 */

#define BOARD_PBL_START start_qemu_malta

#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <asm/pbl_nmon.h>

#include <asm/addrspace.h>
#include <asm/gt64120.h>
#include <mach/mach-gt64120.h>

#ifdef CONFIG_CPU_LITTLE_ENDIAN
#define GT_CPU_TO_LE32(x) (x)
#elif defined CONFIG_CPU_BIG_ENDIAN
#define GT_CPU_TO_LE32(x) ( \
	(((x) & 0x000000ff) << 24) |	\
	(((x) & 0x0000ff00) <<  8) |	\
	(((x) & 0x00ff0000) >>  8) |	\
	(((x) & 0xff000000) >> 24))
#else
#error "could not determine byte order"
#endif

#define	GT_LD(x)	(GT_CPU_TO_LE32(((x) >> 21) & 0x7fff))
#define	GT_HD(x)	(GT_CPU_TO_LE32(((x) >> 21) & 0x7f))

ENTRY_FUNCTION(BOARD_PBL_START)
	b	__start
	 nop

	/*
	   On MIPS Technologies boards
	   0x1fc00010 address is reserved for BoardID
	*/
	.org	0x10
	.word	0xffffffff
	.word	0xffffffff

__start:
	mips_disable_interrupts

	/* cpu specific setup ... */
	/* ... absent */

	/*
	 * Load BAR registers of GT64120 as done by YAMON
	 *
	 * based on write_bootloader() in qemu.git/hw/mips/mips_malta.c
	 * see GT64120 manual and qemu.git/hw/mips/gt64xxx_pci.c for details
	 */

	/* move GT64120 registers to 0x1be00000 */
	li	t1, KSEG1ADDR(GT_DEF_BASE)
	li	t0, GT_LD(MIPS_GT_BASE)
	sw	t0, GT_ISD_OFS(t1)

	/*
	 * setup MEM-to-PCI0 mapping
	 */
	li	t1, KSEG1ADDR(MIPS_GT_BASE)

	/* setup PCI0 io window */
	li	t0, GT_LD(0x18000000)
	sw	t0, GT_PCI0IOLD_OFS(t1)
	li	t0, GT_HD(0x181fffff)
	sw	t0, GT_PCI0IOHD_OFS(t1)

	/* setup PCI0 mem windows */
	li	t0, GT_LD(0x10000000)
	sw	t0, GT_PCI0M0LD_OFS(t1)
	li	t0, GT_HD(0x17efffff)
	sw	t0, GT_PCI0M0HD_OFS(t1)

	li	t0, GT_LD(0x18200000)
	sw	t0, GT_PCI0M1LD_OFS(t1)
	li	t0, GT_LD(0x1bdfffff)
	sw	t0, GT_PCI0M1HD_OFS(t1)

ENTRY_FUNCTION_END(BOARD_PBL_START, qemu_malta)
