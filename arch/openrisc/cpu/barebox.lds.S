/*
 * barebox - barebox.lds.S
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

OUTPUT_FORMAT("elf32-or1k", "elf32-or1k", "elf32-or1k")
__DYNAMIC  =  0;

MEMORY
{
	vectors	: ORIGIN = 0, LENGTH = 0x2000
	ram	: ORIGIN = TEXT_BASE,
		  LENGTH = BAREBOX_RESERVED_SIZE
}

SECTIONS
{
	.vectors :
	{
		*(.vectors)
	} > vectors

	. = ALIGN(4);
	__start = .;
	.text : AT (__start) {
		_stext = .;
		*(.text)
		_etext = .;
		*(.lit)
		*(.shdata)
		_endtext = .;
	}  > ram

	. = ALIGN(4);
	.rodata : {
		*(.rodata);
		*(.rodata.*)
		*(.bbenv.rodata.*)
	} > ram

	. = ALIGN(4);
	. = .;
	__barebox_cmd_start = .;
	.barebox_cmd : { BAREBOX_CMDS } > ram
	__barebox_cmd_end = .;

	__barebox_ratp_cmd_start = .;
	.barebox_ratp_cmd : { BAREBOX_RATP_CMDS } > ram
	__barebox_ratp_cmd_end = .;

	__barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS } > ram
	__barebox_magicvar_end = .;

	__barebox_initcalls_start = .;
	.barebox_initcalls : { INITCALLS } > ram
	__barebox_initcalls_end = .;

	__barebox_exitcalls_start = .;
	.barebox_exitcalls : { EXITCALLS } > ram
	__barebox_exitcalls_end = .;

	___usymtab_start = .;
	__usymtab : { BAREBOX_SYMS } > ram
	___usymtab_end = .;

	.dtb : { BAREBOX_DTB() } > ram

	__etext = .; /* End of text and rodata section */

	. = ALIGN(4);
	.data : {
		sdata = .;
		_sdata = .;
		*(.data)
		edata = .;
		_edata = .;
	} > ram

	. = ALIGN(4);
	.bss :
	{
		__bss_start = .;
		_bss_start = .;
		*(.shbss)
		*(.bss)
		*(COMMON)
		_bss_end = .;
		__bss_stop = .;
	} > ram
	__end = .;
}
