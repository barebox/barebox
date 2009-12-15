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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief A few helper functions for ARM
 */

#include <common.h>
#include <command.h>
#include <asm/mmu.h>

/**
 * Read special processor register
 * @return co-processor 15, register #1 (control register)
 */
static unsigned long read_p15_c1 (void)
{
	unsigned long value;

	__asm__ __volatile__(
		"mrc	p15, 0, %0, c1, c0, 0   @ read control reg\n"
		: "=r" (value)
		:
		: "memory");

#ifdef MMU_DEBUG
	printf ("p15/c1 is = %08lx\n", value);
#endif
	return value;
}

/**
 *
 * Write special processor register
 * @param[in] value to write
 * @return to co-processor 15, register #1 (control register)
 */
static void write_p15_c1 (unsigned long value)
{
#ifdef MMU_DEBUG
	printf ("write %08lx to p15/c1\n", value);
#endif
	__asm__ __volatile__(
		"mcr	p15, 0, %0, c1, c0, 0   @ write it back\n"
		:
		: "r" (value)
		: "memory");

	read_p15_c1 ();
}

/**
 * Wait for co prozessor (waste time)
 * Co processor seems to need some delay between accesses
 */
static void cp_delay (void)
{
	volatile int i;

	for (i = 0; i < 100; i++)	/* FIXME does it work as expected?? */
		;
}

/** mmu off/on */
#define C1_MMU		(1<<0)
/** alignment faults off/on */
#define C1_ALIGN	(1<<1)
/** dcache off/on */
#define C1_DC		(1<<2)
/** big endian off/on */
#define C1_BIG_ENDIAN	(1<<7)
/** system protection */
#define C1_SYS_PROT	(1<<8)
/** ROM protection */
#define C1_ROM_PROT	(1<<9)
/** icache off/on */
#define C1_IC		(1<<12)
/** location of vectors: low/high addresses */
#define C1_HIGH_VECTORS (1<<13)

/**
 * Enable processor's instruction cache
 */
void icache_enable (void)
{
	ulong reg;

	reg = read_p15_c1 ();		/* get control reg. */
	cp_delay ();
	write_p15_c1 (reg | C1_IC);
}

/**
 * Disable processor's instruction cache
 */
void icache_disable (void)
{
	ulong reg;

	reg = read_p15_c1 ();
	cp_delay ();
	write_p15_c1 (reg & ~C1_IC);
}

/**
 * Detect processor's current instruction cache status
 * @return 0=disabled, 1=enabled
 */
int icache_status (void)
{
	return (read_p15_c1 () & C1_IC) != 0;
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
	int i;

	shutdown_barebox();

#ifdef CONFIG_MMU
	mmu_disable();
#endif

	/* flush I/D-cache */
	i = 0;
	asm ("mcr p15, 0, %0, c7, c7, 0": :"r" (i));
	return 0;
}
/**
 * @page arm_boot_preparation Linux Preparation on ARM
 *
 * For ARM we never enable data cache so we do not need to disable it again.
 * Linux can be called with instruction cache enabled. As this is the
 * default setting we are running in barebox, there's no special preparation
 * required.
 */

static int do_icache(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	if (argc == 1) {
		printf("icache is %sabled\n", icache_status() ? "en" : "dis");
		return 0;
	}

	if (simple_strtoul(argv[1], NULL, 0) > 0)
		icache_enable();
	else
		icache_disable();

	return 0;
}

static const __maybe_unused char cmd_icache_help[] =
"Usage: icache [0|1]\n";

BAREBOX_CMD_START(icache)
	.cmd		= do_icache,
	.usage		= "show/change icache status",
	BAREBOX_CMD_HELP(cmd_icache_help)
BAREBOX_CMD_END
