/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2000-2004 Wolfgang Denk <wd@denx.de>, DENX Software Engineering */

#include <asm/barebox.lds.h>
#include <asm/secure.h>

OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)
OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)
ENTRY(start)
SECTIONS
{
	. = 0x0;
	.image_start : { *(.__image_start) }

	. = ALIGN(4);

	._text : { *(._text) }
	.text      :
	{
		_stext = .;
		*(.text_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		. = ALIGN(0x20);
		__exceptions_start = .;
		KEEP(*(.text_exceptions*))
		__exceptions_stop = .;
		*(.text*)
	}
	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4);
	.rodata : {
		*(.rodata*)
		RO_DATA_SECTION
	}

#ifdef CONFIG_ARM_UNWIND
	/*
	 * Stack unwinding tables
	 */
	. = ALIGN(8);
	.ARM.unwind_idx : {
		__start_unwind_idx = .;
		*(.ARM.exidx*)
		__stop_unwind_idx = .;
	}
	.ARM.unwind_tab : {
		__start_unwind_tab = .;
		*(.ARM.extab*)
		__stop_unwind_tab = .;
	}
#endif
	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*)
		CONSTRUCTORS
	}

	.barebox_imd : { BAREBOX_IMD }

	. = .;

	BAREBOX_RELOCATION_TABLE

	_edata = .;
	.image_end : { *(.__image_end) }

	. = ALIGN(4);
	.__bss_start :  { *(.__bss_start) }
	.bss : { *(.bss*) }
	.__bss_stop :  { *(.__bss_stop) }

#ifdef CONFIG_ARM_SECURE_MONITOR
	. = ALIGN(16);
	__secure_stack_start = .;
	. = . + (ARM_SECURE_MAX_CPU << ARM_SECURE_STACK_SHIFT);
	__secure_stack_end = .;
	__secure_end = .;
#endif

	_end = .;
	_barebox_image_size = __bss_start;
}
