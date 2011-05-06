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
#include <cache.h>
#include <asm/mmu.h>
#include <asm/system.h>

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

/**
 * Disable MMU and D-cache, flush caches
 * @return 0 (always)
 *
 * This function is called by shutdown_barebox to get a clean
 * memory/cache state.
 */
void arch_shutdown(void)
{
#ifdef CONFIG_MMU
	/* nearly the same as below, but this could also disable
	 * second level cache.
	 */
	mmu_disable();
#else
	asm volatile (
		"bl __mmu_cache_flush;"
		"bl __mmu_cache_off;"
		:
		:
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "cc", "memory"
	);
#endif
}

/**
 * @page arm_boot_preparation Linux Preparation on ARM
 *
 * For ARM we never enable data cache so we do not need to disable it again.
 * Linux can be called with instruction cache enabled. As this is the
 * default setting we are running in barebox, there's no special preparation
 * required.
 */
#ifdef CONFIG_COMMAND
static int do_icache(struct command *cmdtp, int argc, char *argv[])
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
#endif
