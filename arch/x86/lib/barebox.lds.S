/*
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
 *
 */

#undef i386
#include <asm-generic/barebox.lds.h>

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

MEMORY
{
	mbr(rwx): ORIGIN = TEXT_BASE, LENGTH = 2 * SECTOR_SIZE
	barebox (rwx) : ORIGIN = TEXT_BASE + SECTOR_SIZE, LENGTH = (256 * 1024 * 1024)
}

SECTIONS
{
#ifdef CONFIG_X86_HDBOOT

	.ramlayout : {
		boot_stack = INDIRECT_AREA;
		indirect_area = INDIRECT_AREA;
	}
	/* describing the main boot sector */
	.bootsector : AT (0) {
		*(.boot_start)

		. = 0x00b;
		/*
		 * Maybe later on occupied by a "BIOS parameter block". So,
		 * keep it free from code.
		 * - BytesPerSector dw@0x000B
		 * - SectorsPerCluster db@0x000D
		 * - ReservedSectors dw@0x000E
		 * - FatCopies db@0x0010
		 * - RootDirEntries dw@0x0011
		 * - NumSectors dw@0x0013
		 * - MediaType db@0x0015
		 * - SectorsPerFAT dw@0x0016
		 * - SectorsPerTrack dw@0x0018
		 * - NumberOfHeads dw@0x001A
		 * - HiddenSectors dd@0x001C
		 * - SectorsBig dd@0x0020
		 */
		LONG(0);

		. = 0x024;
		*(.boot_code)
		*(.boot_data)

		/*
		 * embed one "Disk Address Packet Structure" into the boot sector
		 * This DAPS points to the 'indirect' sector to give the boot code
		 * an idea what and where to load. Its content must be adapted
		 * to the system it should run on, so, this structure must be
		 * located at a well known offset.
		 */
		. = PATCH_AREA;
		indirect_sector_lba = .;
		SHORT(0x0010);		/* size of this structure */
		SHORT(0x0001);		/* one sector */
		SHORT(indirect_area);	/* where to store: offset */
		SHORT(0x0000);		/* where to store: segment */
		/* the following values are filled by the installer */
		LONG(0x00000000);	/* LBA start lower */
		LONG(0x00000000);	/* LBA start upper */

		/* boot disk number used by upper layers */
		. = PATCH_AREA + PATCH_AREA_BOOT_DEV;
		boot_disk = .;
		BYTE(0x00);		/* boot disk number (provided by the BIOS)

		/* information about the persistent environment storage */
		. = PATCH_AREA + PATCH_AREA_PERS_START;
		pers_env_storage = .;
		LONG(0x00000000);	/* LBA start lower */
		LONG(0x00000000);	/* LBA start upper */

		. = PATCH_AREA + PATCH_AREA_PERS_SIZE;
		pers_env_size = .;
		SHORT(PATCH_AREA_PERS_SIZE_UNUSED);	/* size of this area in sectors */

		. = PATCH_AREA + PATCH_AREA_PERS_DRIVE;
		pers_env_drive = .;
		BYTE(0x00);		/* used drive */

		/* partition table area (fixed location) */
		. = OFFSET_OF_PARTITION_TABLE;
		/* create an empty one */
		LONG(0x00000000); LONG(0x00000000); LONG(0x00000000); LONG(0x00000000);
		LONG(0x00000000); LONG(0x00000000); LONG(0x00000000); LONG(0x00000000);
		LONG(0x00000000); LONG(0x00000000); LONG(0x00000000); LONG(0x00000000);
		LONG(0x00000000); LONG(0x00000000); LONG(0x00000000); LONG(0x00000000);

		/* boot sector signature */
		. = OFFSET_OF_SIGNATURE;
		BYTE(0x55);
		BYTE(0xAA);
		/* end of the first sector */

		/*
		 * The indirect sector starts here
		 */
		. = SECTOR_SIZE;
		BYTE(MARK_DAPS_INVALID);	/* mark the first entry invalid */
		BYTE(0x00);
		. = SECTOR_SIZE + 496;
		BYTE(MARK_DAPS_INVALID);	/* mark the last entry invalid */
		BYTE(0x00);
		. = SECTOR_SIZE + 508;
		LONG(0x00000000);	/* LBA start upper */
	} > mbr

	/* some real mode bootstrapping */
	.bootstrapping : AT ( LOADADDR(.bootsector) + SIZEOF(.bootsector) ) {
		*(.boot.head)
		*(.boot.text*)
		*(.boot.rodata*)
		*(.boot.data*)
		. = ALIGN(4);
	} > barebox
#endif

	/* the main barebox part (32 bit) */
	.text : AT ( LOADADDR(.bootstrapping) + SIZEOF(.bootstrapping) ) {
		/* do not align here! It may fails with the LOADADDR! */
		_stext = .;
		_text = .;
		*(.text_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
		. = ALIGN(4);
		*(.rodata*)
		. = ALIGN(4);
		_etext = .;			/* End of text and rodata section */
	} > barebox
	BAREBOX_BARE_INIT_SIZE

	_sdata = .;
	.data : AT ( LOADADDR(.text) + SIZEOF(.text) ) {
		*(.data*)
		. = ALIGN(4);
	} > barebox

	.got : AT ( LOADADDR(.data) + SIZEOF (.data) ) {
		*(.got*)
		. = ALIGN(4);
	} > barebox

	.barebox_cmd : AT ( LOADADDR(.got) + SIZEOF (.got) ) {
		__barebox_cmd_start = .;
		BAREBOX_CMDS
		__barebox_cmd_end = .;
		. = ALIGN(4);
	} > barebox

	.barebox_ratp_cmd : AT ( LOADADDR(.got) + SIZEOF (.got) ) {
		__barebox_ratp_cmd_start = .;
		BAREBOX_RATP_CMDS
		__barebox_ratp_cmd_end = .;
		. = ALIGN(4);
	} > barebox

	.barebox_magicvars : AT ( LOADADDR(.barebox_cmd) + SIZEOF (.barebox_cmd) ) {
		__barebox_magicvar_start = .;
		BAREBOX_MAGICVARS
		__barebox_magicvar_end = .;
		. = ALIGN(4);
	} > barebox

	.barebox_initcalls : AT ( LOADADDR(.barebox_magicvars) + SIZEOF (.barebox_magicvars) ) {
		__barebox_initcalls_start = .;
		INITCALLS
		__barebox_initcalls_end = .;
		. = ALIGN(4);
	} > barebox

	.barebox_exitcalls : AT ( LOADADDR(.barebox_initcalls) + SIZEOF (.barebox_initcalls) ) {
		__barebox_exitcalls_start = .;
		EXITCALLS
		__barebox_exitcalls_end = .;
		. = ALIGN(4);
	} > barebox

	.__usymtab : AT ( LOADADDR(.barebox_exitcalls) + SIZEOF (.barebox_exitcalls) ) {
		__usymtab_start = .;
		BAREBOX_SYMS
		__usymtab_end = .;
		. = ALIGN(4);
	} > barebox

	_edata = .;
	.bss : {
		__bss_start = .;
		*(.bss*);
		*( COMMON )
		__bss_stop = .;
		_end = .;
	} > barebox
}
