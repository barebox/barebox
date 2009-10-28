/**
 * @file
 * @brief Generic Clock wrapper header.
 *
 * FileName: include/asm-arm/arch-omap/clocks.h
 *
 * This includes each of the architecture Clock definitions under it.
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 */
/*
 * (C) Copyright 2006-2008
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
#ifndef __OMAP_CLOCKS_H_
#define __OMAP_CLOCKS_H_

#define LDELAY		12000000

/* Standard defines for Various clocks */
#define S12M		12000000
#define S13M		13000000
#define S19_2M		19200000
#define S24M		24000000
#define S26M		26000000
#define S38_4M		38400000

#ifdef CONFIG_ARCH_OMAP3
#include <mach/omap3-clock.h>
#endif

#endif /* __OMAP_CLOCKS_H_ */
