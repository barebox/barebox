/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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

/**
 * @file
 * @brief Adapt linker script content in accordance to Kconfig settings
 */

/**
 * Area in the MBR of the barebox basic boot code. This offset must be in
 * accordance to the 'indirect_sector_lba' label.
 */
#define PATCH_AREA 400

/**
 * Offset where to store the boot drive number (BIOS number, 1 byte)
 */
#define PATCH_AREA_BOOT_DEV 16

/**
 * Offset where to store information about the persistant environment storage
 * It points to an LBA number (8 bytes) and defines the first sector of this
 * storage on disk.
 */
#define PATCH_AREA_PERS_START 20

/**
 * Offset where to store information about the persistant environment storage
 * It points to a short number (2 bytes) and defines the sector count of this
 * storage on disk.
 */
#define PATCH_AREA_PERS_SIZE 28

/**
 * Offset where to store information about the persistant environment storage
 * drive number (BIOS number, 1 byte)
 */
#define PATCH_AREA_PERS_DRIVE 30

/**
 * Mark the persistant environment as not used
 */
#define PATCH_AREA_PERS_SIZE_UNUSED 0x000

/**
 * Mark a DAPS as unused/invalid
 */
#define MARK_DAPS_INVALID 0x0000

/**
 * Offset of the partition table in an MBR
 */
#define OFFSET_OF_PARTITION_TABLE 446

/**
 * Offset of the signature in an MBR
 */
#define OFFSET_OF_SIGNATURE 510

/**
 * Area where to store indirect sector to loop through. Keep this value
 * in accordance to the 'indirect_area' label. Note: .
 *
 * @attention These addresses are real mode ones (seg:offset)
 */
#define INDIRECT_AREA 0x7A00
#define INDIRECT_SEGMENT 0x0000

/**
 * Area where to load sectors from disk to. They should start after the
 * MBR area and must be in accordance to the offset of the '.bootstrapping'
 * section in the linker file.
 *
 * @attention The address must be a multiple of 512.
 */
#define LOAD_AREA 0x7e00
#define LOAD_SEGMENT 0x0000

/**
 * Size of one sector.
 */
#define SECTOR_SIZE 512
