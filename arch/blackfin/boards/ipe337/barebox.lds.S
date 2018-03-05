/*
 * barebox - barebox.lds.S
 *
 * Copyright (c) 2005-2007 Analog Device Inc.
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <config.h>
#include <asm-generic/barebox.lds.h>

OUTPUT_ARCH("bfin")
SEARCH_DIR(/lib); SEARCH_DIR(/usr/lib); SEARCH_DIR(/usr/local/lib);
/* Do we need any of these for elf?
   __DYNAMIC = 0;    */
/*
MEMORY
{
	ram : 	   ORIGIN = (0x2000000), LENGTH = (256 * 1024)
	l1_code : ORIGIN = 0xFFA00000, LENGTH = 0xC000
	l1_data : ORIGIN = 0xFF900000, LENGTH = 0x4000
}
*/

SECTIONS
{
	. = TEXT_BASE;

	. = ALIGN(4);
	.text      :
	{
		__stext = .;
		__text = .;
		_text = .;
		*(.text_entry)
		*(.text)
	}

	. = ALIGN(4);
	.rodata : { *(.rodata) }

	__etext = .;			/* End of text and rodata section */
	__sdata = .;

	. = ALIGN(4);
	.data : { *(.data) }

	. = ALIGN(4);
	.got : { *(.got) }

	. = .;
	___barebox_cmd_start = .;
	.barebox_cmd : { BAREBOX_CMDS }
	___barebox_cmd_end = .;

	___barebox_ratp_cmd_start = .;
	.barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
	___barebox_ratp_cmd_end = .;

	___barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	___barebox_magicvar_end = .;

	___barebox_initcalls_start = .;
	.barebox_initcalls : { INITCALLS }
	___barebox_initcalls_end = .;

	___barebox_exitcalls_start = .;
	.barebox_exitcalls : { EXITCALLS }
	___barebox_exitcalls_end = .;

	___usymtab_start = .;
	__usymtab : { BAREBOX_SYMS }
	___usymtab_end = .;

	__edata = .;
	. = ALIGN(4);
	___bss_start = .;
	.bss : { *(.bss) }
	___bss_stop = .;
	_end = .;
}
