// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix

/**
 * @file
 * @brief Definition of the Global Descriptor Table
 */

#include <types.h>
#include <asm/modes.h>
#include <asm/segment.h>

/**
 * The 'Global Descriptor Table' used in barebox
 *
 * Note: This table must reachable by real and flat mode code
 */
uint64_t gdt[] __attribute__((aligned(16))) __bootdata = {
	/* CS: code, read/execute, 4 GB, base 0 */
	[GDT_ENTRY_BOOT_CS] = GDT_ENTRY(0xc09b, 0, 0xfffff),
	/* DS: data, read/write, 4 GB, base 0 */
	[GDT_ENTRY_BOOT_DS] = GDT_ENTRY(0xc093, 0, 0xfffff),
	/* CS: for real mode calls */
	[GDT_ENTRY_REAL_CS] = GDT_ENTRY(0x009E, 0, 0x0ffff),
	/* DS: for real mode calls */
	[GDT_ENTRY_REAL_DS] = GDT_ENTRY(0x0092, 0, 0x0ffff),
	/* TSS: 32-bit tss, 104 bytes, base 4096 */
	/* We only have a TSS here to keep Intel VT happy;
	   we don't actually use it for anything. */
	[GDT_ENTRY_BOOT_TSS] = GDT_ENTRY(0x0089, 4096, 103),
};

/**
 * Size of the GDT must be known to load it
 *
 * Note: This varibale must reachable by real and flat mode code
 */
unsigned gdt_size __bootdata = sizeof(gdt);
