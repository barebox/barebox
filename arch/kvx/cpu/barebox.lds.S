/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <asm/common.h>
#include <asm/sys_arch.h>
#include <asm/barebox.lds.h>

OUTPUT_FORMAT("elf64-kvx")

SECTIONS
{
	. = CONFIG_ARCH_TEXT_BASE;
	.text ALIGN(4) : {
		*(.startup);
		_stext = .;
		*(.text)
	}

	/* Exception vector must be aligned on a huge frontier */
	.exception ALIGN(EXCEPTION_ALIGNMENT) :
	{
		_exception_start = ABSOLUTE(.);
		/**
		 * First handler is at _exception_start + EXCEPTION_STRIDE
		 * In order to force getting to the next stride, add at
		 * least 1 byte of data. The next ALIGN will then be
		 * forced to get to the next stride.
		 */
		. += 1;
		. = ALIGN(EXCEPTION_STRIDE);

		/* Entry for traps */
		KEEP(*(.exception.trap));
		. += 1;

		/* Entry for interrupts */
		. = ALIGN(EXCEPTION_STRIDE);
		KEEP(*(.exception.interrupt));
		. += 1;

		/* Entry for syscall */
		. = ALIGN(EXCEPTION_STRIDE);
		KEEP(*(.exception.syscall));
	}

	__start_rodata = .;

	.rodata ALIGN(8) : {
		*(.rodata*)
		. = ALIGN(8);
		RO_DATA_SECTION
	}

	__end_rodata = .;
	_etext = .;

	.data ALIGN(4): {
		sdata = .;
		_sdata = .;
		*(.data)
		. = ALIGN(8);
		__stack_end = .;
		. += CONFIG_STACK_SIZE;
		__stack_start = .;
	}

	.gdb_page ALIGN(4 * 1024) :
	{
		_debug_start = ABSOLUTE(.);
		_debug_phy_start = ABSOLUTE(.);
		. += 4 * 1024;
	}
	__debug_phy_end = ABSOLUTE(.);
	_edata = .;

	/* We use store quad for bss init so align on 16 bytes */
	.bss ALIGN(16):
	{
		__bss_start = .;
		*(.bss)
		. = ALIGN(16);
		__bss_stop = .;
	}
	__end = .;
}
