/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix */

#include <linux/sizes.h>
#include <asm/barebox.lds.h>
#include <asm/memory.h>
#include <asm-generic/memory_layout.h>
#include <asm-generic/pointer.h>
#include <asm/memory.h>

/*
 * The size of the PE/COFF section that covers the barebox image, which
 * runs from _stext to _edata, must be a round multiple of the PE/COFF
 * FileAlignment, which we set to its minimum value of 0x200. '_stext'
 * itself must be 4 KB aligned, because that's what the adrp instructions
 * expects, so padding out _edata to a 0x200 aligned boundary should be
 * sufficient.
 */
PECOFF_FILE_ALIGNMENT = 0x200;

#ifdef CONFIG_EFI_STUB
#define PECOFF_EDATA_PADDING   \
       .pecoff_edata_padding : { BYTE(0); . = ALIGN(PECOFF_FILE_ALIGNMENT); }
#else
#define PECOFF_EDATA_PADDING
#endif

#ifdef CONFIG_PBL_RELOCATABLE
#define BASE	0x0
#else
#define BASE	(TEXT_BASE - SZ_2M)
#endif

#ifdef CONFIG_HABV4_QSPI
#define HAB_CSF_LEN	0x4000
#else
#define HAB_CSF_LEN	0x2000
#endif

OUTPUT_FORMAT(BAREBOX_OUTPUT_FORMAT)
OUTPUT_ARCH(BAREBOX_OUTPUT_ARCH)

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
		*(.text_head_prologue*)
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

	. = ALIGN(ASM_SZPTR);
	__pbl_board_stack_top = .;
	.rodata.pbl_board_stack_top : {
		*(.pbl_board_stack_top_*)
		/* Dummy for when BootROM sets up usable stack */
		ASM_LD_PTR(0x00000000)
	}
	ASSERT(. - __pbl_board_stack_top <= 2 * ASM_SZPTR, "Only One PBL per Image allowed")

	.barebox_imd : { BAREBOX_IMD }

	. = ALIGN(PBL_SEGMENT_ALIGN);
	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*) }

	. = ALIGN(4);
	__shasum_start = .;
	.shasum : {
		KEEP(*(.shasum))
	}
	__shasum_end = .;

	BAREBOX_RELOCATION_TABLE

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
		. += + HAB_CSF_LEN - 1;
	} = 0x5a
	__csf_end = .;
#endif /* CONFIG_CPU_64 && CONFIG_HABV4 */

	. = ALIGN(4);
	__piggydata_start = .;
	.piggydata : {
		*(.piggydata)
	}

	. = ALIGN(4);
	__pblext_start = .;
	.pblext : {
		*(.pblext.*)
	}
	__pblext_end = .;

	PECOFF_EDATA_PADDING

	__pecoff_data_rawsize = ABSOLUTE(. - _etext);

	/* .bss is dwarfed by piggydata size, so we just handle .bss
	 * as normal PE data
	 */

	__pecoff_data_size = ABSOLUTE(. - _etext);

	.image_end : { KEEP(*(.__image_end)) }

	pbl_image_size =  . - BASE;

	_barebox_image_size = __image_end - BASE;
	_barebox_pbl_size = __bss_start - BASE;
}
