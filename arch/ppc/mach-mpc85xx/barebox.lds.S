/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
 * Copyright 2007-2009, 2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <asm-generic/barebox.lds.h>

#ifdef CONFIG_RESET_VECTOR_ADDRESS
#define RESET_VECTOR_ADDRESS    CONFIG_RESET_VECTOR_ADDRESS
#else
#define RESET_VECTOR_ADDRESS    0xfffffffc
#endif

OUTPUT_ARCH("powerpc")
ENTRY(_start_e500)

PHDRS
{
  text PT_LOAD;
  bss PT_LOAD;
}

SECTIONS
{
  . = TEXT_BASE;
   _stext = .;
   PROVIDE (stext = .);

  .interp        : { *(.interp)		}
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .rel.text      : { *(.rel.text)	}
  .rela.text     : { *(.rela.text)	}
  .rel.data      : { *(.rel.data)	}
  .rela.data     : { *(.rela.data)	}
  .rel.rodata    : { *(.rel.rodata)	}
  .rela.rodata   : { *(.rela.rodata)}
  .rel.got       : { *(.rel.got)	}
  .rela.got      : { *(.rela.got)	}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.bss       : { *(.rel.bss)	}
  .rela.bss      : { *(.rela.bss)	}
  .rel.plt       : { *(.rel.plt)	}
  .rela.plt      : { *(.rela.plt)	}
  .init          : { *(.init)		}
  .plt           : { *(.plt)		}
  .text      :
  {
    *(.text*)
    *(.got1*)

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
    KEEP(*(.got))
    _GOT2_TABLE_ = .;
    KEEP(*(.got2))
    PROVIDE(_GLOBAL_OFFSET_TABLE_ = . + 4);
    _FIXUP_TABLE_ = .;
    KEEP(*(.fixup))
  }
  __got2_entries = ((_FIXUP_TABLE_ - _GOT2_TABLE_) >> 2);
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

  . = .;
  __barebox_cmd_start = .;
  .barebox_cmd : { BAREBOX_CMDS }
  __barebox_cmd_end = .;

  __barebox_ratp_cmd_start = .;
  .barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
  __barebox_ratp_cmd_end = .;

  __barebox_initcalls_start = .;
  .barebox_initcalls : { INITCALLS }
  __barebox_initcalls_end = .;
  __initcall_entries = (__barebox_initcalls_end - __barebox_initcalls_start)>>2;

  __barebox_exitcalls_start = .;
  .barebox_exitcalls : { EXITCALLS }
  __barebox_exitcalls_end = .;
  __exitcall_entries = (__barebox_exitcalls_end - __barebox_exitcalls_start) >> 2;

  __usymtab_start = .;
  __usymtab : { BAREBOX_SYMS }
  __usymtab_end = .;

  . = .;
  __start___ex_table = .;
  __ex_table : { *(__ex_table) }
  __stop___ex_table = .;

  _edata  =  .;
  PROVIDE (edata = .);

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
    arch/ppc/cpu-85xx/start.o	(.bootpg)
  } :text = 0xffff

  .resetvec RESET_VECTOR_ADDRESS :
  {
    KEEP(*(.resetvec))
    arch/ppc/cpu-85xx/resetvec.o	(.resetvec)
  } :text = 0xffff

  . = RESET_VECTOR_ADDRESS + 0x4;

#if (RESET_VECTOR_ADDRESS == 0xfffffffc)
  /* This avoids wrapping around to offset 0 */
  . |= 0x10;
#endif

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
