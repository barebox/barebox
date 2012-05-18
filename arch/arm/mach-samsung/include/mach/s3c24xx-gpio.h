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

#ifndef __MACH_GPIO_S3C24X0_H
# define __MACH_GPIO_S3C24X0_H

#define S3C_GPACON (S3C_GPIO_BASE)
#define S3C_GPADAT (S3C_GPIO_BASE + 0x04)

#define S3C_GPBCON (S3C_GPIO_BASE + 0x10)
#define S3C_GPBDAT (S3C_GPIO_BASE + 0x14)
#define S3C_GPBUP (S3C_GPIO_BASE + 0x18)

#define S3C_GPCCON (S3C_GPIO_BASE + 0x20)
#define S3C_GPCDAT (S3C_GPIO_BASE + 0x24)
#define S3C_GPCUP (S3C_GPIO_BASE + 0x28)

#define S3C_GPDCON (S3C_GPIO_BASE + 0x30)
#define S3C_GPDDAT (S3C_GPIO_BASE + 0x34)
#define S3C_GPDUP (S3C_GPIO_BASE + 0x38)

#define S3C_GPECON (S3C_GPIO_BASE + 0x40)
#define S3C_GPEDAT (S3C_GPIO_BASE + 0x44)
#define S3C_GPEUP (S3C_GPIO_BASE + 0x48)

#define S3C_GPFCON (S3C_GPIO_BASE + 0x50)
#define S3C_GPFDAT (S3C_GPIO_BASE + 0x54)
#define S3C_GPFUP (S3C_GPIO_BASE + 0x58)

#define S3C_GPGCON (S3C_GPIO_BASE + 0x60)
#define S3C_GPGDAT (S3C_GPIO_BASE + 0x64)
#define S3C_GPGUP (S3C_GPIO_BASE + 0x68)

#define S3C_GPHCON (S3C_GPIO_BASE + 0x70)
#define S3C_GPHDAT (S3C_GPIO_BASE + 0x74)
#define S3C_GPHUP (S3C_GPIO_BASE + 0x78)

#ifdef CONFIG_CPU_S3C2440
# define S3C_GPJCON (S3C_GPIO_BASE + 0xd0)
# define S3C_GPJDAT (S3C_GPIO_BASE + 0xd4)
# define S3C_GPJUP (S3C_GPIO_BASE + 0xd8)
#endif

#define S3C_MISCCR  (S3C_GPIO_BASE + 0x80)
#define S3C_DCLKCON (S3C_GPIO_BASE + 0x84)
#define S3C_EXTINT0 (S3C_GPIO_BASE + 0x88)
#define S3C_EXTINT1 (S3C_GPIO_BASE + 0x8c)
#define S3C_EXTINT2 (S3C_GPIO_BASE + 0x90)
#define S3C_EINTFLT0 (S3C_GPIO_BASE + 0x94)
#define S3C_EINTFLT1 (S3C_GPIO_BASE + 0x98)
#define S3C_EINTFLT2 (S3C_GPIO_BASE + 0x9c)
#define S3C_EINTFLT3 (S3C_GPIO_BASE + 0xa0)
#define S3C_EINTMASK (S3C_GPIO_BASE + 0xa4)
#define S3C_EINTPEND (S3C_GPIO_BASE + 0xa8)
#define S3C_GSTATUS0 (S3C_GPIO_BASE + 0xac)
#define S3C_GSTATUS1 (S3C_GPIO_BASE + 0xb0)
#define S3C_GSTATUS2 (S3C_GPIO_BASE + 0xb4)
#define S3C_GSTATUS3 (S3C_GPIO_BASE + 0xb8)
#define S3C_GSTATUS4 (S3C_GPIO_BASE + 0xbc)

#ifdef CONFIG_CPU_S3C2440
# define S3C_DSC0 (S3C_GPIO_BASE + 0xc4)
# define S3C_DSC1 (S3C_GPIO_BASE + 0xc8)
#endif

#endif /* __MACH_GPIO_S3C24X0_H */
