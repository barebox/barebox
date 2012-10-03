/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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

#ifdef CONFIG_ARCH_IMX23
# define cpu_is_mx23()	(1)
#else
# define cpu_is_mx23()	(0)
#endif

#ifdef CONFIG_ARCH_IMX28
# define cpu_is_mx28()	(1)
#else
# define cpu_is_mx28()	(0)
#endif

#define cpu_is_mx1()	(0)
#define cpu_is_mx21()	(0)
#define cpu_is_mx25()	(0)
#define cpu_is_mx27()	(0)
#define cpu_is_mx31()	(0)
#define cpu_is_mx35()	(0)
#define cpu_is_mx51()	(0)
#define cpu_is_mx53()	(0)
#define cpu_is_mx6()	(0)
