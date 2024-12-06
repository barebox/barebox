/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/barebox.lds.h>

OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)
OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
ENTRY(_start)
SECTIONS
{
	. = 0;
	image_base = .;
	.hash : { *(.hash) }	/* this MUST come first! */
	. = ALIGN(4096);
	.text :
	{
		_stext = .;
		_text = .;
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)
	}

	_etext = .;

	. = ALIGN(4096);
	.sdata : {
		*(.got.plt)
		*(.got)
		*(.srodata)
		*(.sdata)
		*(.sbss)
		*(.scommon)
	}

	. = ALIGN(4096);
	_sdata = .;

	.data : {
		__start_rodata = .;
		*(.rodata*)
		RO_DATA_SECTION
		__end_rodata = .;
		*(.data)
		*(.data1)
		*(.data.*)
		*(.sdata)
		*(.got.plt)
		*(.got)
		/* the EFI loader doesn't seem to like a .bss section, so we stick
		 * it all into .data: */
		*(.sbss)
		*(.scommon)
		*(.dynbss)
		*(.bss)
		*(COMMON)
	}

	. = ALIGN(4096);
	.dynamic : { *(.dynamic) }
	. = ALIGN(4096);
	.rel : {
		*(.rel.data)
		*(.rel.data.*)
		*(.rel.got)
		*(.rel.stab)
		*(.data.rel.ro.local)
		*(.data.rel.local)
		*(.data.rel.ro)
		*(.data.rel*)
	}

	. = ALIGN(4096);
	.reloc :		/* This is the PECOFF .reloc section! */
	{
		*(.reloc)
	}

	. = ALIGN(4096);
	.dynsym : { *(.dynsym) }
	. = ALIGN(4096);
	.dynstr : { *(.dynstr) }
	. = ALIGN(4096);
	/DISCARD/ :
	{
		*(.rel.reloc)
		*(.eh_frame)
		*(.note.GNU-stack)
	}

	.comment 0 : { *(.comment) }
}
