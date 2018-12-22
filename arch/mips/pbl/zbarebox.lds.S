/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <asm-generic/barebox.lds.h>
#include <asm-generic/memory_layout.h>

OUTPUT_ARCH("mips")
ENTRY(pbl_start)
SECTIONS
{
	. = HEAD_TEXT_BASE;

	PRE_IMAGE

	. = ALIGN(4);
	.text      :
	{
		_stext = .;
		_text = .;
		*(.text_head_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
	}

	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4);
	.rodata : { *(.rodata*) }

	.barebox_imd : { BAREBOX_IMD }

	_etext = .;			/* End of text and rodata section */

	. = ALIGN(4);
	.data : { *(.data*) }

	. = ALIGN(4);
	__bss_start = .;
	.bss : { *(.bss*) }
	__bss_stop = .;
	_end = .;
}
