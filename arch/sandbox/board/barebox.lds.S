/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/barebox.lds.h>

SECTIONS
{
	. = ALIGN(64);
	.barebox_rodata : {
		RO_DATA_SECTION
	}
}

INSERT BEFORE .rodata;
