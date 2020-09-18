// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix

/**
 * @file
 * @brief Start of the 32 bit flat mode
 */

#include <string.h>
#include <asm/sections.h>

extern void x86_start_barebox(void);

/**
 * Called plainly from assembler that switches from real to flat mode
 *
 * @note The C environment isn't initialized yet
 */
void uboot_entry(void)
{
	/* clear the BSS first */
	memset(__bss_start, 0x00, __bss_stop - __bss_start);
	x86_start_barebox();
}
