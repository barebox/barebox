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
 * @brief Declarations to bring some light in the real/protected/flat mode darkness
 */
#ifndef _ASM_X86_MODES_H
#define _ASM_X86_MODES_H

#ifndef __ASSEMBLY__

#include <types.h>

extern uint64_t gdt[];
extern unsigned gdt_size;

#endif

/** to simplify GDT entry generation */
#define GDT_ENTRY(flags, base, limit)			\
	((((base)  & 0xff000000ULL) << (56-24)) |	\
	 (((flags) & 0x0000f0ffULL) << 40) |		\
	 (((limit) & 0x000f0000ULL) << (48-16)) |	\
	 (((base)  & 0x00ffffffULL) << 16) |		\
	 (((limit) & 0x0000ffffULL)))

/** 32 bit barebox text */
#define GDT_ENTRY_BOOT_CS	2
#define __BOOT_CS		(GDT_ENTRY_BOOT_CS * 8)

/** 32 bit barebox data */
#define GDT_ENTRY_BOOT_DS	3
#define __BOOT_DS		(GDT_ENTRY_BOOT_DS * 8)

/** 16 bit barebox text */
#define GDT_ENTRY_REAL_CS	4
#define __REAL_CS		(GDT_ENTRY_REAL_CS * 8)

/** 16 bit barebox data */
#define GDT_ENTRY_REAL_DS	5
#define __REAL_DS		(GDT_ENTRY_REAL_DS * 8)

/** Something to make others happy */
#define GDT_ENTRY_BOOT_TSS	6
#define __BOOT_TSS		(GDT_ENTRY_BOOT_TSS * 8)

#endif /* _ASM_X86_MODES_H */
