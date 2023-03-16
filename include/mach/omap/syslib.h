/**
 * @file
 * @brief These Apis are OMAP independent support functions
 *
 * Implemented by arch/arm/mach-omap/syslib.c
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 *
 * (C) Copyright 2004-2008
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
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

#ifndef __ASM_ARCH_OMAP_SYSLIB_H_
#define __ASM_ARCH_OMAP_SYSLIB_H_
#include <io.h>

/** System Independent functions */

/**
 * @brief clear & set a value in a bit range for a 32 bit address
 *
 * @param[in] addr Address to set/read from
 * @param[in] start_bit Where to put the value
 * @param[in] num_bits number of bits the value should be set
 * @param[in] value the value to set
 *
 * @return void
 */
static inline void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value)
{
	u32 tmp, msk = 0;
	msk = 1 << num_bits;
	--msk;
	tmp = readl(addr) & ~(msk << start_bit);
	tmp |=  value << start_bit;
	writel(tmp, addr);
}

u32 wait_on_value(u32 read_bit_mask, u32 match_value, u32 read_addr, u32 bound);
void sdelay(unsigned long loops);

#endif /* __ASM_ARCH_OMAP_SYSLIB_H_ */
