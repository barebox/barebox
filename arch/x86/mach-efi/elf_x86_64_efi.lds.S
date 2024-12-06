/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/barebox.lds.h>

/* Same as elf_x86_64_fbsd_efi.lds, except for OUTPUT_FORMAT below - KEEP IN SYNC */

OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)
OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
ENTRY(_start)
SECTIONS
{
	. = 0;
	image_base = .;
	.hash : { *(.hash) }	/* this MUST come first! */
	. = ALIGN(4096);
	.eh_frame : {
		*(.eh_frame)
	}

	. = ALIGN(4096);

	.text : {
		_stext = .;
		_text = .;
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)
	}

	_etext = .;

	. = ALIGN(4096);

	.reloc : {
		*(.reloc)
	}

	. = ALIGN(4096);
	_sdata = .;

	.data : {
		__start_rodata = .;
		*(.rodata*)
		RO_DATA_SECTION
		__end_rodata = .;
		*(.got.plt)
		*(.got)
		*(.data*)
		__bss_start = .;
		*(.sdata)
		/* the EFI loader doesn't seem to like a .bss section, so we stick
		 * it all into .data: */
		*(.sbss)
		*(.scommon)
		*(.dynbss)
		*(.bss)
		*(COMMON)
		*(.rel.local)
		__bss_stop = .;
	}
	_edata = .;

	.barebox_imd : { BAREBOX_IMD }

	. = ALIGN(4096);
	.dynamic : { *(.dynamic) }
	. = ALIGN(4096);

	.rela : {
		*(.rela*)
	}

	. = ALIGN(4096);
	.dynsym : { *(.dynsym) }
	. = ALIGN(4096);
	.dynstr : { *(.dynstr) }
	. = ALIGN(4096);
	.ignored.reloc : {
		*(.rela.reloc)
		*(.eh_frame)
		*(.note.GNU-stack)
	}

	.comment 0 : { *(.comment) }
}
