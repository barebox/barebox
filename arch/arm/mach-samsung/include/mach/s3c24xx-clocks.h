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

# define S3C_LOCKTIME (S3C_CLOCK_POWER_BASE)
# define S3C_MPLLCON (S3C_CLOCK_POWER_BASE + 0x4)
# define S3C_UPLLCON (S3C_CLOCK_POWER_BASE + 0x8)
# define S3C_CLKCON (S3C_CLOCK_POWER_BASE + 0xc)
# define S3C_CLKSLOW (S3C_CLOCK_POWER_BASE + 0x10)
# define S3C_CLKDIVN (S3C_CLOCK_POWER_BASE + 0x14)

# define S3C_MPLLCON_GET_MDIV(x) ((((x) >> 12) & 0xff) + 8)
# define S3C_MPLLCON_GET_PDIV(x) ((((x) >> 4) & 0x3f) + 2)
# define S3C_MPLLCON_GET_SDIV(x) ((x) & 0x3)
