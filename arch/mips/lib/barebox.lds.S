/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */


#include <asm-generic/barebox.lds.h>

OUTPUT_ARCH(mips)
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
	.rodata : { *(.rodata*) }

	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*) }

	.barebox_imd : { BAREBOX_IMD }

	. = .;
	.barebox_cmd : { BAREBOX_CMDS }
	.barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	.barebox_initcalls : { BAREBOX_INITCALLS }
	.barebox_exitcalls : { BAREBOX_EXITCALLS }
	__usymtab : { BAREBOX_SYMS }
	.pci_fixup : { BAREBOX_PCI_FIXUP }
	.oftables : { BAREBOX_CLK_TABLE }
	.dtb : { BAREBOX_DTB }

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
		. += (32 * 1024) - 1;
	}

	_end = .;

	.bss __rel_start (OVERLAY) : {
		__bss_start = .;
		*(.sbss.*)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_stop = .;
	}

}
