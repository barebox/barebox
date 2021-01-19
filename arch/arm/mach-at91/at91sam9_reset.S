/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2010 BitBox Ltd */

/*
 * reset AT91SAM9G20 as per errata
 *
 * unless the SDRAM is cleanly shutdown before we hit the
 * reset register it can be left driving the data bus and
 * killing the chance of a subsequent boot from NAND
 */

#include <linux/linkage.h>
#include <mach/at91sam9_sdramc.h>
#include <mach/at91_rstc.h>

			.arm

			.globl	at91sam9_reset

at91sam9_reset:		mov	r2, #1
			mov	r3, #AT91_SDRAMC_LPCB_POWER_DOWN
			ldr	r4, =AT91_RSTC_KEY | AT91_RSTC_PERRST | AT91_RSTC_PROCRST

			.balign	32						@ align to cache line

			str	r2, [r0, #AT91_SDRAMC_TR]			@ disable SDRAM access
			str	r3, [r0, #AT91_SDRAMC_LPR]			@ power down SDRAM
			str	r4, [r1]					@ reset processor

			b	.
