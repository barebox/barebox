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

#ifndef __MACH_S3C_CLOCKS_H
#define __MACH_S3C_CLOCKS_H

#ifdef CONFIG_ARCH_S3C24xx
# include <mach/s3c24xx-clocks.h>
#endif
#ifdef CONFIG_ARCH_S3C64xx
# include <mach/s3c64xx-clocks.h>
#endif
#ifdef CONFIG_ARCH_S5PCxx
# include <mach/s5pcxx-clocks.h>
#endif

#endif /* __MACH_S3C_CLOCKS_H */
