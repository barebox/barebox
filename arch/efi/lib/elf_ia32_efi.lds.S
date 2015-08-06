#include <asm-generic/barebox.lds.h>

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
	. = 0;
	image_base = .;
	.hash : { *(.hash) }	/* this MUST come first! */
	. = ALIGN(4096);
	.text :
	{
		_stext = .;
		_text = .;
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)
	}

	_etext = .;

	. = ALIGN(4096);
	.sdata : {
		*(.got.plt)
		*(.got)
		*(.srodata)
		*(.sdata)
		*(.sbss)
		*(.scommon)
	}

	. = ALIGN(4096);
	_sdata = .;

	.data : {
		*(.rodata*)
		*(.data)
		*(.data1)
		*(.data.*)
		*(.sdata)
		*(.got.plt)
		*(.got)
		/* the EFI loader doesn't seem to like a .bss section, so we stick
		 * it all into .data: */
		*(.sbss)
		*(.scommon)
		*(.dynbss)
		*(.bss)
		*(COMMON)
	}

	. = ALIGN(64);

	__barebox_initcalls_start = .;
	__barebox_initcalls : { INITCALLS }
	__barebox_initcalls_end = .;

	__barebox_exitcalls_start = .;
	__barebox_exitcalls : { EXITCALLS }
	__barebox_exitcalls_end = .;

	. = ALIGN(64);
	__barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	__barebox_magicvar_end = .;

	. = ALIGN(64);
	__barebox_cmd_start = .;
	__barebox_cmd : { BAREBOX_CMDS }
	__barebox_cmd_end = .;

	. = ALIGN(4096);
	.dynamic : { *(.dynamic) }
	. = ALIGN(4096);
	.rel : {
		*(.rel.data)
		*(.rel.data.*)
		*(.rel.got)
		*(.rel.stab)
		*(.data.rel.ro.local)
		*(.data.rel.local)
		*(.data.rel.ro)
		*(.data.rel*)
	}

	. = ALIGN(4096);
	.reloc :		/* This is the PECOFF .reloc section! */
	{
		*(.reloc)
	}

	. = ALIGN(4096);
	.dynsym : { *(.dynsym) }
	. = ALIGN(4096);
	.dynstr : { *(.dynstr) }
	. = ALIGN(4096);
	/DISCARD/ :
	{
		*(.rel.reloc)
		*(.eh_frame)
		*(.note.GNU-stack)
	}

	.comment 0 : { *(.comment) }
}
