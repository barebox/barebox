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

	. = ALIGN(64);
	__barebox_ratp_cmd_start = .;
	__barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
	__barebox_ratp_cmd_end = .;

	. = ALIGN(4096);
	.dynamic : { *(.dynamic) }
	. = ALIGN(4096);

	.rela : {
		*(.rela.data*)
		*(.rela.barebox*)
		*(.rela.initcall*)
		*(.rela.exitcall*)
		*(.rela.got)
		*(.rela.stab)
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
