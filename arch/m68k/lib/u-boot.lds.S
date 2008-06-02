/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of U-Boot V2.
 *
 * U-Boot V2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * U-Boot V2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with U-Boot V2.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Generic Linker file for M68k targets
 */
#include <asm-generic/u-boot.lds.h>

OUTPUT_FORMAT("elf32-m68k", "elf32-m68k",
	      "elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)
SECTIONS
{
	. = TEXT_BASE;
	. = ALIGN(4);

	/* Start of vector, text and rodata section */
	_stext = .;
	_text = .;

	/* M68k/CF style vector table */
	.vectors :
	{
		*(.vectors)
	}

	.text :
	{
		*(.text .stub .text.*)
	} =0x4e754e75

	. = ALIGN(4);
	.rodata :
	{
		*(.rodata .rodata.*)
	} =0xdeadbeef

	. = ALIGN(4);
	__u_boot_cmd_start = .;
	.u_boot_cmd : { U_BOOT_CMDS }
	__u_boot_cmd_end = .;

	__u_boot_initcalls_start = .;
	.u_boot_initcalls : { INITCALLS }
	__u_boot_initcalls_end = .;

	__usymtab_start = .;
	__usymtab : { U_BOOT_SYMS }
	__usymtab_end = .;

	/* End of text and rodata section */
	. = ALIGN(4);
	_etext = .;

	. = ALIGN(4);
	.got : { *(.got) }
	. = ALIGN(4);

	. = ALIGN(4);
	__early_init_data_begin = .;
	.early_init_data : { *(.early_init_data) }
	__early_init_data_end = .;

	.data : { *(.data .data.*) }

	. = ALIGN(4);
	__bss_start = .;
	.bss (NOLOAD) : { *(.bss .bass.*) }
	__bss_end =.;
	_end = .;

	. = ALIGN(4);
	_u_boot_heap_start = .;
}
