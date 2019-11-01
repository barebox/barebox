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
 */
#include <linux/sizes.h>
#include <asm-generic/barebox.lds.h>
#include <asm-generic/memory_layout.h>

#ifdef CONFIG_PBL_RELOCATABLE
#define BASE	0x0
#else
#define BASE	(TEXT_BASE - SZ_2M)
#endif

#ifdef CONFIG_CPU_32
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
#else
OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
#endif
SECTIONS
{
	. = BASE;

	.image_start : { *(.__image_start) }

	PRE_IMAGE

	. = ALIGN(4);
	._text : { *(._text) }
	.text      :
	{
		_stext = .;
		*(.text_head_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
	}

	/* Discard unwind if enable in barebox */
	/DISCARD/ : { *(.ARM.ex*) }

	BAREBOX_BARE_INIT_SIZE
	BAREBOX_PBL_SIZE

	. = ALIGN(4);
	.rodata : { *(.rodata*) }

	.barebox_imd : { BAREBOX_IMD }

	_etext = .;			/* End of text and rodata section */

	. = ALIGN(4);
	.data : { *(.data*) }

	. = ALIGN(4);
	__shasum_start = .;
	.shasum : {
		KEEP(*(.shasum))
	}
	__shasum_end = .;

	.rel_dyn_start : { *(.__rel_dyn_start) }
#ifdef CONFIG_CPU_32
	.rel.dyn : { *(.rel*) }
#else
	.rela.dyn : { *(.rela*) }
#endif
	.rel_dyn_end : { *(.__rel_dyn_end) }

	.__dynsym_start :  { *(.__dynsym_start) }
	.dynsym : { *(.dynsym) }
	.__dynsym_end : { *(.__dynsym_end) }

	pbl_code_size =  . - BASE;

	. = ALIGN(4);
	.__bss_start :  { *(.__bss_start) }
	.bss : { *(.bss*) }
	.__bss_stop :  { *(.__bss_stop) }
	_end = .;

	pbl_memory_size =  . - BASE;

#if defined(CONFIG_CPU_64) && defined(CONFIG_HABV4)
	. = ALIGN(0x1000);
	__csf_start = .;
	.hab_csf : {
		BYTE(0x5a);
		. += + 0x1fff;
	} = 0x5a
	__csf_end = .;
#endif /* CONFIG_CPU_64 && CONFIG_HABV4 */

	. = ALIGN(4);
	__piggydata_start = .;
	.piggydata : {
		*(.piggydata)
	}
	__piggydata_end = .;

	.image_end : { KEEP(*(.__image_end)) }

	pbl_image_size =  . - BASE;

	_barebox_image_size = __image_end - BASE;
	_barebox_pbl_size = __bss_start - BASE;
}
