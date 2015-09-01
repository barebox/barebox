/*
 * cpu.c - A few helper functions for ARM
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file
 * @brief A few helper functions for ARM
 */

#include <common.h>
#include <init.h>
#include <command.h>
#include <cache.h>
#include <asm/mmu.h>
#include <asm/system.h>
#include <asm/memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/cputype.h>
#include <asm/cache.h>
#include <asm/ptrace.h>

#include "mmu.h"

/**
 * Enable processor's instruction cache
 */
void icache_enable(void)
{
	u32 r;

	r = get_cr();
	r |= CR_I;
	set_cr(r);
}

/**
 * Disable processor's instruction cache
 */
void icache_disable(void)
{
	u32 r;

	r = get_cr();
	r &= ~CR_I;
	set_cr(r);
}

/**
 * Detect processor's current instruction cache status
 * @return 0=disabled, 1=enabled
 */
int icache_status(void)
{
	return (get_cr () & CR_I) != 0;
}

/*
 * SoC like the ux500 have the l2x0 always enable
 * with or without MMU enable
 */
struct outer_cache_fns outer_cache;

/*
 * Clean and invalide caches, disable MMU
 */
void mmu_disable(void)
{
	__mmu_cache_flush();
	if (outer_cache.disable)
		outer_cache.disable();
	__mmu_cache_off();
}

/**
 * Disable MMU and D-cache, flush caches
 * @return 0 (always)
 *
 * This function is called by shutdown_barebox to get a clean
 * memory/cache state.
 */
static void arch_shutdown(void)
{
	uint32_t r;

	mmu_disable();
	flush_icache();
	/*
	 * barebox normally does not use interrupts, but some functionalities
	 * (eg. OMAP4_USBBOOT) require them enabled. So be sure interrupts are
	 * disabled before exiting.
	 */
	__asm__ __volatile__("mrs %0, cpsr" : "=r"(r));
	r |= PSR_I_BIT;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r));
}
archshutdown_exitcall(arch_shutdown);

extern unsigned long arm_stack_top;

static int arm_request_stack(void)
{
	if (!request_sdram_region("stack", arm_stack_top - STACK_SIZE, STACK_SIZE))
		pr_err("Error: Cannot request SDRAM region for stack\n");

	return 0;
}
coredevice_initcall(arm_request_stack);

#ifdef CONFIG_THUMB2_BAREBOX
static void thumb2_execute(void *func, int argc, char *argv[])
{
	/*
	 * Switch back to ARM mode before executing external
	 * programs.
	 */
	__asm__ __volatile__ (
		"mov r0, #0\n"
		"mov r1, %0\n"
		"mov r2, %1\n"
		"bx %2\n"
		:
		: "r" (argc - 1), "r" (&argv[1]), "r" (func)
		: "r0", "r1", "r2"
	);
}

static int execute_init(void)
{
	do_execute = thumb2_execute;
	return 0;
}
postcore_initcall(execute_init);
#endif
