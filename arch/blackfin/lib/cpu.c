/*
 * barebox - cpu.c CPU specific functions
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#include <common.h>
#include <asm/blackfin.h>
#include <command.h>
#include <asm/entry.h>
#include <asm/cpu.h>
#include <init.h>

void __noreturn reset_cpu(unsigned long addr)
{
	icache_disable();

	__asm__ __volatile__
	("cli r3;"
	"P0 = %0;"
	"JUMP (P0);"
	:
	: "r" (L1_ISRAM)
	);

	/* Not reached */
	while (1);
}

void icache_disable(void)
{
#ifdef __ADSPBF537__
	if ((*pCHIPID >> 28) < 2)
		return;
#endif
	__builtin_bfin_ssync();
	asm(" .align 8; ");
	*(unsigned int *)IMEM_CONTROL &= ~(IMC | ENICPLB);
	__builtin_bfin_ssync();
}

void icache_enable(void)
{
	unsigned int *I0, *I1;
	int j = 0;
#ifdef __ADSPBF537__
	if ((*pCHIPID >> 28) < 2)
		return;
#endif
	/* Before enable icache, disable it first */
	icache_disable();

	I0 = (unsigned int *)ICPLB_ADDR0;
	I1 = (unsigned int *)ICPLB_DATA0;

	/* We only setup instruction caching for barebox itself.
	 * This has the nice side effect that we trigger an
	 * exception when barebox goes crazy.
	 */
	*I0++ = TEXT_BASE & ~((1 << 20) - 1);
	*I1++ = PAGE_SIZE_1MB | CPLB_L1_CHBL | CPLB_USER_RD | CPLB_VALID | CPLB_LOCK;
	j++;

	/* Fill the rest with invalid entry */
	for ( ; j < 16 ; j++) {
		debug("filling %i with 0\n",j);
		*I1++ = 0x0;
	}

	__builtin_bfin_ssync();
	asm(" .align 8; ");
	*(unsigned int *)IMEM_CONTROL = IMC | ENICPLB;
	__builtin_bfin_ssync();
}

int icache_status(void)
{
	unsigned int value;
	value = *(unsigned int *)IMEM_CONTROL;

	if (value & (IMC | ENICPLB))
		return 1;
	else
		return 0;
}

static void blackfin_init_exceptions(void)
{
	*(unsigned volatile long *) (SIC_IMASK) = SIC_UNMASK_ALL;
#ifndef CONFIG_KGDB
	*(unsigned volatile long *) (EVT_EMULATION_ADDR) = 0x0;
#endif
	*(unsigned volatile long *) (EVT_NMI_ADDR) =
		(unsigned volatile long) evt_nmi;
	*(unsigned volatile long *) (EVT_EXCEPTION_ADDR) =
		(unsigned volatile long) trap;
	*(unsigned volatile long *) (EVT_HARDWARE_ERROR_ADDR) =
		(unsigned volatile long) evt_ivhw;
	*(volatile unsigned long *) ILAT = 0;
	asm("csync;");
	*(volatile unsigned long *) IMASK = 0x3f;
	asm("csync;");
}

static int blackfin_init_core(void)
{
	blackfin_init_exceptions();
	icache_enable();

	return 0;
}

core_initcall(blackfin_init_core);

