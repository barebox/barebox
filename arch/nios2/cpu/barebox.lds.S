/*
 * barebox - barebox.lds.S
 *
 * (C) Copyright 2011, Franck JULLIEN, <elec4fun@gmail.com>
 *
 * (C) Copyright 2004, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
 *
 * Copyright (c) 2005-2007 Analog Device Inc.
 *
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
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

OUTPUT_FORMAT("elf32-littlenios2")
OUTPUT_ARCH("nios2")

SECTIONS
{
	. = NIOS_SOPC_TEXT_BASE;

	. = ALIGN(4);
	.text      :
	{
		_stext = .;
		__text = .;
		_text = .;
		__bare_init_start = .;
		*(.text_entry)
		__bare_init_end = .;
		*(.text)
	}
	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4);
	.rodata : { *(.rodata) }

	. = .;
	__barebox_cmd_start = .;
	.barebox_cmd : { BAREBOX_CMDS }
	__barebox_cmd_end = .;

	__barebox_ratp_cmd_start = .;
	.barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
	__barebox_ratp_cmd_end = .;

	__barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	__barebox_magicvar_end = .;

	__barebox_initcalls_start = .;
	.barebox_initcalls : { INITCALLS }
	__barebox_initcalls_end = .;

	__barebox_exitcalls_start = .;
	.barebox_exitcalls : { EXITCALLS }
	__barebox_exitcalls_end = .;

	___usymtab_start = .;
	__usymtab : { BAREBOX_SYMS }
	___usymtab_end = .;

	_etext = .; /* End of text and rodata section */

	/* INIT DATA sections - "Small" data (see the gcc -G option)
	 * is always gp-relative. Here we make all init data sections
	 * adjacent to simplify the startup code -- and provide
	 * the global pointer for gp-relative access.
	 */
	_sdata = .;
	_data = .;
	.data :
	{
	  *(.data)
	  *(.data.*)
	  *(.gnu.linkonce.d*)
	}

	. = ALIGN(16);
	_gp = .;            /* Global pointer addr */
	PROVIDE (gp = .);

	.sdata :
	{
	  *(.sdata)
	  *(.sdata.*)
	  *(.gnu.linkonce.s.*)
	}
	. = ALIGN(4);

	_edata = .;
	PROVIDE (edata = .);

	/* UNINIT DATA - Small uninitialized data is first so it's
	 * adjacent to sdata and can be referenced via gp. The normal
	 * bss follows. We keep it adjacent to simplify init code.
	 */
	__bss_start = .;
	.sbss (NOLOAD) :
	{
	  *(.sbss)
	  *(.sbss.*)
	  *(.gnu.linkonce.sb.*)
	  *(.scommon)
	}
	. = ALIGN(4);
	.bss (NOLOAD) :
	{
	  *(.bss)
	  *(.bss.*)
	  *(.dynbss)
	  *(COMMON)
	  *(.scommon)
	}
	__bss_stop = .;

	. = ALIGN(4);
	_end = .;
	PROVIDE (end = .);
}
