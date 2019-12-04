#include <asm-generic/barebox.lds.h>

/* Same as elf_x86_64_fbsd_efi.lds, except for OUTPUT_FORMAT below - KEEP IN SYNC */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
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
		*(.rodata*)
		RO_DATA_SECTION
		*(.got.plt)
		*(.got)
		*(.data*)
		*(.sdata)
		/* the EFI loader doesn't seem to like a .bss section, so we stick
		 * it all into .data: */
		*(.sbss)
		*(.scommon)
		*(.dynbss)
		*(.bss)
		*(COMMON)
		*(.rel.local)
	}

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
