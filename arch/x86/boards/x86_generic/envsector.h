/*
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
 * @brief x86 Generic PC common definitions
 */
#ifndef __X86_ENVSECTOR_H
#define __ENVSECTOR_H

/*
 * These datas are from the MBR, created by the linker and filled by the
 * setup tool while installing barebox on the disk drive
 */
extern uint64_t pers_env_storage;
extern uint16_t pers_env_size;
extern uint8_t pers_env_drive;

/**
 * Persistent environment "not used" marker.
 * Note: Must be in accordance to the value the tool "setup_mbr" writes.
 */
#define PATCH_AREA_PERS_SIZE_UNUSED 0x000

#endif
