/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2000-2004 Wolfgang Denk <wd@denx.de>, DENX Software Engineering */

#include <asm/barebox.lds.h>

OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)
OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
ENTRY(start)
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
		*(.text*)
	}
	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4096);
	__start_rodata = .;
	.rodata : {
		*(.rodata*)
		RO_DATA_SECTION
	}

	. = ALIGN(4096);

	__end_rodata = .;
	_etext = .;
	_sdata = .;

	.data : { *(.data*) }

	BAREBOX_RELOCATION_TABLE

	_edata = .;

	BAREBOX_EFI_RUNTIME

	.image_end : { *(.__image_end) }

	. = ALIGN(4);
	.__bss_start :  { *(.__bss_start) }
	.bss : { *(.bss*) }
	.__bss_stop :  { *(.__bss_stop) }
	_end = .;
	_barebox_image_size = __bss_start;
}

NOCROSSREFS_FROM(.efi_runtime)
