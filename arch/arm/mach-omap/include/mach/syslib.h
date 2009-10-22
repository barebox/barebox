/**
 * @file
 * @brief These Apis are OMAP independent support functions
 *
 * FileName: include/asm-arm/arch-omap/syslib.h
 *
 * Implemented by arch/arm/mach-omap/syslib.c
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 */
/*
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __ASM_ARCH_OMAP_SYSLIB_H_
#define __ASM_ARCH_OMAP_SYSLIB_H_

/** System Independent functions */
void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value);
u32 wait_on_value(u32 read_bit_mask, u32 match_value, u32 read_addr, u32 bound);
void sdelay(unsigned long loops);

/** All architectures need to implement these */
void omap_uart_write(unsigned int val, unsigned long base,
		     unsigned char reg_idx);
unsigned int omap_uart_read(unsigned long base, unsigned char reg_idx);
#endif /* __ASM_ARCH_OMAP_SYSLIB_H_ */
