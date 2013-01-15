/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2007-2009, 2011 Freescale Semiconductor, Inc.
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

#include <asm-generic/barebox.lds.h>

#define RESET_VECTOR_ADDRESS	0xeffffffc

OUTPUT_ARCH("powerpc")

PHDRS
{
  text PT_LOAD;
  bss PT_LOAD;
}

SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = TEXT_BASE;

  .text      :
  {
    *(.text*)
  } :text

  _etext = .;
  PROVIDE (etext = .);
  _sdata = .;

  .rodata    :
  {
    *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
  } :text

  /* Read-write section, merged into data segment: */
  . = (. + 0x00FF) & 0xFFFFFF00;

  _erotext = .;
  PROVIDE (erotext = .);

  .reloc   :
  {
    _GOT2_TABLE_ = .;
    KEEP(*(.got2))
    KEEP(*(.got))
    PROVIDE(_GLOBAL_OFFSET_TABLE_ = . + 4);
    _FIXUP_TABLE_ = .;
    KEEP(*(.fixup))
  }
  __got2_entries = ((_GLOBAL_OFFSET_TABLE_ - _GOT2_TABLE_) >> 2) - 1;
  __fixup_entries = (. - _FIXUP_TABLE_) >> 2;

  .data    :
  {
    *(.data*)
    *(.data1*)
    *(.sdata*)
    *(.sdata2*)
    *(.dynamic*)
    CONSTRUCTORS
  }
  _edata  =  .;
  PROVIDE (edata = .);

  . = .;
  __barebox_cmd_start = .;
  .barebox_cmd : { BAREBOX_CMDS }
  __barebox_cmd_end = .;

  __barebox_initcalls_start = .;
  .barebox_initcalls : { INITCALLS }
  __barebox_initcalls_end = .;
  __initcall_entries = (__barebox_initcalls_end - __barebox_initcalls_start)>>2;

  __usymtab_start = .;
  __usymtab : { BAREBOX_SYMS }
  __usymtab_end = .;

  __early_init_data_begin = .;
  .early_init_data : { *(.early_init_data) }
  __early_init_data_end = .;

  . = .;
  __start___ex_table = .;
  __ex_table : { *(__ex_table) }
  __stop___ex_table = .;

  . = ALIGN(256);
  __init_begin = .;
  .text.init : { *(.text.init) }
  .data.init : { *(.data.init) }
  . = ALIGN(256);
  __init_end = .;

  __init_size = __init_end - _start;

  .bootpg RESET_VECTOR_ADDRESS - 0xffc :
  {
    _text = .;
    _stext = .;
    arch/ppc/cpu-85xx/start.o	(.bootpg)
  } :text = 0xffff

  .resetvec RESET_VECTOR_ADDRESS :
  {
    arch/ppc/cpu-85xx/resetvec.o	(.resetvec)
  } :text = 0xffff

  . = RESET_VECTOR_ADDRESS + 0x4;

  . = 0x10000;
  __bss_start = .;
  .bss        :
  {
    *(.sbss*) *(.scommon*)
    *(.dynbss*)
    *(.bss*)
    *(COMMON)
  } :bss

  . = ALIGN(4);
  __bss_stop = .;
  _end = . ;
  PROVIDE (end = .);
}
