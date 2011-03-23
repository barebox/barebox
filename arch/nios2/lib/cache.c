/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009, Wind River Systems Inc
 * Implemented by fredrik.markstrom@gmail.com and ivarholmqvist@gmail.com
 */

#include <config.h>

static void __flush_dcache(unsigned long start, unsigned long end)
{
	unsigned long addr;

	start &= ~(DCACHE_LINE_SIZE - 1);
	end += (DCACHE_LINE_SIZE - 1);
	end &= ~(DCACHE_LINE_SIZE - 1);

	if (end > start + DCACHE_SIZE)
		end = start + DCACHE_SIZE;

	for (addr = start; addr < end; addr += DCACHE_LINE_SIZE) {
		__asm__ __volatile__ ("   flushd 0(%0)\n"
					: /* Outputs */
					: /* Inputs  */ "r"(addr)
					/* : No clobber */);
	}
}

static void __flush_icache(unsigned long start, unsigned long end)
{
	unsigned long addr;

	start &= ~(ICACHE_LINE_SIZE - 1);
	end += (ICACHE_LINE_SIZE - 1);
	end &= ~(ICACHE_LINE_SIZE - 1);

	if (end > start + ICACHE_SIZE)
		end = start + ICACHE_SIZE;

	for (addr = start; addr < end; addr += ICACHE_LINE_SIZE) {
		__asm__ __volatile__ ("   flushi %0\n"
					: /* Outputs */
					: /* Inputs  */ "r"(addr)
					/* : No clobber */);
	}
	__asm__ __volatile(" flushp\n");
}

void flush_dcache_all(void)
{
	__flush_dcache(0, DCACHE_SIZE);
}

void flush_icache_all(void)
{
	__flush_icache(0, ICACHE_SIZE);
}

void flush_cache_all(void)
{
	flush_dcache_all();
	flush_icache_all();
}

void flush_icache_range(unsigned long start, unsigned long end)
{
	__flush_icache(start, end);
}

void flush_dcache_range(unsigned long start, unsigned long end)
{
	__flush_dcache(start, end);
	/* FIXME: Maybe we should remove __flush_icache ? */
	__flush_icache(start, end);
}
