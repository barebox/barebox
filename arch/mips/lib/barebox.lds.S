/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <asm/barebox.lds.h>

OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
ENTRY(_start)
SECTIONS
{
	. = TEXT_BASE;

	. = ALIGN(4);

	.image_start : { *(.__image_start) }
	.text      :
	{
		_stext = .;
		_text = .;
		*(.text_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
	}
	BAREBOX_BARE_INIT_SIZE

	PRE_IMAGE

	. = ALIGN(4);
	.rodata : {
		*(.rodata*)
		RO_DATA_SECTION
	}

	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*) }

	.barebox_imd : { BAREBOX_IMD }

	_edata = .;
	.image_end : { *(.__image_end) }

	. = ALIGN(4);
	/*
	 * .rel must come last so that the mips-relocs tool can shrink
	 * the section size & the PT_LOAD program header filesz.
	 */
	.data.reloc : {
		__rel_start = .;
		BYTE(0x0)
		. += CONFIG_MIPS_RELOCATION_TABLE_SIZE - 1;
	}

	_end = .;

	.bss : {
		__bss_start = .;
		*(.sbss.*)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_stop = .;
	}
}
