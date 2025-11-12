/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/barebox.lds.h>

OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
ENTRY(start)
OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)
SECTIONS
{
	. = 0x0;

	.image_start : { *(.__image_start) }

	. = ALIGN(4);
	._text : { *(._text) }
	.text      :
	{
		_stext = .;
		*(.text_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		__exceptions_start = .;
		KEEP(*(.text_exceptions*))
		__exceptions_stop = .;
		*(.text*)
	}
	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4);
	__start_rodata = .;
	.rodata : {
		*(.rodata*)
		RO_DATA_SECTION
	}

	__end_rodata = .;
	_etext = .;
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*) }

	/DISCARD/ : { *(.rela.plt*) }
	.rela.dyn : {
		__rel_dyn_start = .;
		*(.rel*)
		__rel_dyn_end = .;
	}

	.dynsym : {
		__dynsym_start = .;
		*(.dynsym)
		__dynsym_end = .;
	}

	_edata = .;

	.image_end : { *(.__image_end) }

	. = ALIGN(4);
	.__bss_start :  { *(.__bss_start) }
	.bss : { *(.bss*) }
	.__bss_stop :  { *(.__bss_stop) }
	_end = .;
	_barebox_image_size = __bss_start;
}
