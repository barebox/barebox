/*
 * (C) Copyright 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
#include <sizes.h>
#include <asm-generic/barebox.lds.h>
#include <asm-generic/memory_layout.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SECTIONS
{
#ifdef CONFIG_PBL_RELOCATABLE
	. = 0x0;
#else
	. = TEXT_BASE - SZ_2M;
#endif

	PRE_IMAGE

	. = ALIGN(4);
	.text      :
	{
		_stext = .;
		_text = .;
		*(.text_head_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
	}

	/* Discard unwind if enable in barebox */
	/DISCARD/ : { *(.ARM.ex*) }

	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4);
	.rodata : { *(.rodata*) }

	_etext = .;			/* End of text and rodata section */

	. = ALIGN(4);
	.data : { *(.data*) }

	.rel.dyn : {
		__rel_dyn_start = .;
		*(.rel*)
		__rel_dyn_end = .;
	}

	.dynsym : {
		__dynsym_start = .;
		*(.dynsym)
		__dynsym_end = .;
	}

	. = ALIGN(4);
	__bss_start = .;
	.bss : { *(.bss*) }
	__bss_stop = .;
	_end = .;

	. = ALIGN(4);
	__piggydata_start = .;
	.piggydata : {
		*(.piggydata)
	}
	__piggydata_end = .;

	. = ALIGN(4);
	.image_end : {
		KEEP(*(.image_end))
	}
	__image_end = .;

	_barebox_image_size = __image_end - (TEXT_BASE - SZ_2M);
	_barebox_pbl_size = __bss_start - (TEXT_BASE - SZ_2M);
}
