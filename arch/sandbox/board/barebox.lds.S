/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/barebox.lds.h>

SECTIONS
{
	. = ALIGN(64);
	__start_rodata = .;
	.barebox_rodata () : {
		RO_DATA_SECTION
	}
	__end_rodata = .;
}

INSERT BEFORE .data;
