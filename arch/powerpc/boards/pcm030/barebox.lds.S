/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#include <asm/barebox.lds.h>

OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
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
    arch/powerpc/mach-mpc5xxx/start.o	(.text)
    *(.text*)
    *(.got1*)
    . = ALIGN(16);
    __start_rodata = .;
    *(.rodata*)
    *(.rodata1*)
    *(.rodata.str1.4)
    RO_DATA_SECTION
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
  __end_rodata = .;
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
