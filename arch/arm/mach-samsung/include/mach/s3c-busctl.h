/*
 * Copyright (C) 2011 Juergen Beisert, Pengutronix
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
 */

#ifndef __MACH_S3C_BUSCTL_H
# define __MACH_S3C_BUSCTL_H

#define S3C_BWSCON (S3C_MEMCTL_BASE)
#define S3C_BANKCON0 (S3C_MEMCTL_BASE + 0x04)
#define S3C_BANKCON1 (S3C_MEMCTL_BASE + 0x08)
#define S3C_BANKCON2 (S3C_MEMCTL_BASE + 0x0c)
#define S3C_BANKCON3 (S3C_MEMCTL_BASE + 0x10)
#define S3C_BANKCON4 (S3C_MEMCTL_BASE + 0x14)
#define S3C_BANKCON5 (S3C_MEMCTL_BASE + 0x18)
#define S3C_BANKCON6 (S3C_MEMCTL_BASE + 0x1c)
#define S3C_BANKCON7 (S3C_MEMCTL_BASE + 0x20)
#define S3C_REFRESH (S3C_MEMCTL_BASE + 0x24)
#define S3C_BANKSIZE (S3C_MEMCTL_BASE + 0x28)
#define S3C_MRSRB6 (S3C_MEMCTL_BASE + 0x2c)
#define S3C_MRSRB7 (S3C_MEMCTL_BASE + 0x30)

#endif /* __MACH_S3C_BUSCTL_H */
