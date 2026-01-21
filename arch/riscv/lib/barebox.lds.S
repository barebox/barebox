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
ENTRY(barebox_non_pbl_start)
OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)

PHDRS
{
	text PT_LOAD FLAGS(5);		/* PF_R | PF_X */
	rodata PT_LOAD FLAGS(4);	/* PF_R */
	dynamic PT_DYNAMIC FLAGS(4);	/* PF_R */
	data PT_LOAD FLAGS(6);		/* PF_R | PF_W */
}

SECTIONS
{
	. = 0x0;

	.image_start : { *(.__image_start) } :text

	. = ALIGN(4);
	._text : { *(._text) } :text
	.text      :
	{
		_stext = .;
		__exceptions_start = .;
		KEEP(*(.text_exceptions*))
		__exceptions_stop = .;
		*(.text*)
	} :text

	. = ALIGN(4096);
	__start_rodata = .;
	.rodata : {
		*(.rodata*)
		RO_DATA_SECTION
	} :rodata

	.dynamic : { *(.dynamic) } :rodata :dynamic

	__end_rodata = .;
	_etext = .;
	_sdata = .;

	. = ALIGN(4096);

	.data : { *(.data*) } :data

	/DISCARD/ : { *(.rela.plt*) }
	.rela.dyn : {
		__rel_dyn_start = .;
		*(.rel*)
		__rel_dyn_end = .;
	} :data

	.dynsym : {
		__dynsym_start = .;
		*(.dynsym)
		__dynsym_end = .;
	} :data

	_edata = .;

	.image_end : { *(.__image_end) } :data

	. = ALIGN(4);
	.__bss_start :  { *(.__bss_start) } :data
	.bss : { *(.bss*) } :data
	.__bss_stop :  { *(.__bss_stop) } :data
	_end = .;
	_barebox_image_size = __bss_start;
}
