/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  A few helper functions for M6kK/Coldfire
 */
#include <common.h>
#include <command.h>
#include <init.h>
#include <proc/processor.h> // FIXME -stup place
#include <mach/mcf54xx-regs.h>

static uint32_t CACR_shadow = MCF5XXX_CACR_BEC;

/*
 * Reset init value := 0x010C0100
 *	MCF5XXX_CACR_DCINVA
 *	MCF5XXX_CACR_BEC
 *	MCF5XXX_CACR_BCINVA
 *	MCF5XXX_CACR_ICINVA
 */

/**
 * Enable processor's instruction cache
 */
void icache_enable (void)
{
	CACR_shadow |= MCF5XXX_CACR_IEC;
	mcf5xxx_wr_cacr( CACR_shadow );
}

/**
 * Disable processor's instruction cache
 */
void icache_disable (void)
{
	CACR_shadow &= ~MCF5XXX_CACR_IEC;
	mcf5xxx_wr_cacr( CACR_shadow );
}

/**
 * Detect processor's current instruction cache status
 * @return 0=disabled, 1=enabled
 */
int icache_status (void)
{
	return (CACR_shadow & MCF5XXX_CACR_IEC)?1:0;
}

/**
 * Enable processor's data cache
 */
void dcache_enable (void)
{
	CACR_shadow |= MCF5XXX_CACR_DEC;
	mcf5xxx_wr_cacr( CACR_shadow );
}

/**
 * Disable processor's data cache
 */
void dcache_disable (void)
{
	CACR_shadow &= ~MCF5XXX_CACR_DEC;
	mcf5xxx_wr_cacr( CACR_shadow );
}

/**
 * Detect processor's current instruction cache status
 * @return 0=disabled, 1=enabled
 */
int dcache_status (void)
{
	return (CACR_shadow & MCF5XXX_CACR_DEC)?1:0;
}

/**
 * Flush CPU caches to memory
 */
void cpu_cache_flush(void)
{
	uint32_t way, set;
	void *addr;

	for ( way=0; way < 4; way++ ) {
		addr = (void*)way;
		for ( set=0; set < 512; set++ ) {
			mcf5xxx_cpushl_bc ( addr );
			addr += 0x10;
		}
	}
}

/**
 * Flush CPU caches to memory and disable them.
 */
void cpu_cache_disable(void)
{
	uint32_t lastipl;

	lastipl = asm_set_ipl( 7 );

	cpu_cache_flush();
	mcf5xxx_wr_acr0( 0 );
	mcf5xxx_wr_acr1( 0 );
	mcf5xxx_wr_acr2( 0 );
	mcf5xxx_wr_acr3( 0 );

	CACR_shadow &= ~MCF5XXX_CACR_IEC;
	CACR_shadow &= ~MCF5XXX_CACR_DEC;
	mcf5xxx_wr_cacr( CACR_shadow | (MCF5XXX_CACR_DCINVA|MCF5XXX_CACR_ICINVA));

	lastipl = asm_set_ipl( lastipl );
}

/**
 * Prepare a "clean" CPU for Linux to run
 * @return 0 (always)
 *
 * This function is called by the generic barebox part just before we call
 * Linux. It prepares the processor for Linux.
 */
int cleanup_before_linux (void)
{
	/*
	 * we never enable dcache so we do not need to disable
	 * it. Linux can be called with icache enabled, so just
	 * do nothing here
	 */

	/* flush I/D-cache */
	cpu_cache_disable();

	/* reenable icache */
	icache_enable();
	return (0);
}
/** @page m68k_boot_preparation Linux Preparation on M68k/Coldfire
 *
 * For M68K we never enable data cache so we do not need to disable it again.
 *
 * Linux can be called with instruction cache enabled. As this is the
 * default setting we are running in barebox, there's no special preparation
 * required.
 */


/** Early init of Coldfire V4E CPU
 */
static int cpu_init (void)
{
	/* Enable ICache - branch cache is already on */
	icache_enable();

	/*
	 * setup up stacks if necessary
	 * setup other CPU specifics here to prepare
	 * handling of exceptions and interrupts
	 */
#ifdef CONFIG_USE_IRQ
	printf("Prepare CPU interrupts for handlers\n");
	mcf_interrupts_initialize();
#endif

	return 0;
}

core_initcall(cpu_init);
