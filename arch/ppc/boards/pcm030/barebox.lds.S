/*
 * (C) Copyright 2003
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

#include <asm-generic/barebox.lds.h>

OUTPUT_ARCH("powerpc")
ENTRY(_start)
/* Do we need any of these for elf?
   __DYNAMIC = 0;    */
SECTIONS
{
  . = TEXT_BASE;

  .text      :
  {
    _text = .;
    _stext = .;
    arch/ppc/mach-mpc5xxx/start.o	(.text)
    *(.text*)
    *(.got1*)
    . = ALIGN(16);
    *(.rodata*)
    *(.rodata1*)
    *(.rodata.str1.4)
  }

  /* Read-only sections, merged into text segment: */
/*
  .interp : { *(.interp) }
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .rel.text      : { *(.rel.text)		}
  .rela.text     : { *(.rela.text) 	}
  .rel.data      : { *(.rel.data)		}
  .rela.data     : { *(.rela.data) 	}
  .rel.rodata    : { *(.rel.rodata) 	}
  .rela.rodata   : { *(.rela.rodata) 	}
  .rel.got       : { *(.rel.got)		}
  .rela.got      : { *(.rela.got)		}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  .init          : { *(.init)	}
  .plt : { *(.plt) }
  .text      :
  .fini      : { *(.fini)    } =0
  .ctors     : { *(.ctors)   }
  .dtors     : { *(.dtors)   }
*/

  /* Read-write section, merged into data segment: */
  . = (. + 0x0FFF) & 0xFFFFF000;
  _etext = .;
  PROVIDE (erotext = .);
  _sdata = .;
  .reloc   :
  {
    *(.got)
    _GOT2_TABLE_ = .;
    *(.got2)
    _FIXUP_TABLE_ = .;
    *(.fixup)
  }
  __got2_entries = (_FIXUP_TABLE_ - _GOT2_TABLE_) >> 2;
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

  __barebox_ratp_cmd_start = .;
  .barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
  __barebox_ratp_cmd_end = .;

  __barebox_magicvar_start = .;
  .barebox_magicvar : { BAREBOX_MAGICVARS }
  __barebox_magicvar_end = .;

  __barebox_initcalls_start = .;
  .barebox_initcalls : { INITCALLS }
  __barebox_initcalls_end = .;
  __initcall_entries = (__barebox_initcalls_end - __barebox_initcalls_start) >> 2;

  __barebox_exitcalls_start = .;
  .barebox_exitcalls : { EXITCALLS }
  __barebox_exitcalls_end = .;
  __exitcall_entries = (__barebox_exitcalls_end - __barebox_exitcalls_start) >> 2;

  __usymtab_start = .;
  __usymtab : { BAREBOX_SYMS }
  __usymtab_end = .;

  __start___ex_table = .;
  __ex_table : { *(__ex_table) }
  __stop___ex_table = .;

  . = ALIGN(4096);
  __init_begin = .;
  .text.init : { *(.text.init) }
  .data.init : { *(.data.init) }
  . = ALIGN(4096);
  __init_end = .;

  __init_size = __init_end - _start;

  __bss_start = .;
  .bss       :
  {
   *(.sbss*) *(.scommon*)
   *(.dynbss*)
   *(.bss*)
   *(COMMON)
  }
  __bss_stop = .;
  _end = . ;
  PROVIDE (end = .);
}
