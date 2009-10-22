/**
 * @file
 * @brief This file defines the macros apis which are useful for most OMAP
 * platforms.
 *
 * FileName: include/asm-arm/arch-omap/sys_info.h
 *
 * These are implemented by the System specific code in omapX-generic.c
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

#ifndef __ASM_ARCH_SYS_INFO_H_
#define __ASM_ARCH_SYS_INFO_H_

#define XDR_POP		5      /* package on package part */
#define SDR_DISCRETE	4      /* 128M memory SDR module*/
#define DDR_STACKED	3      /* stacked part on 2422 */
#define DDR_COMBO	2      /* combo part on cpu daughter card (menalaeus) */
#define DDR_DISCRETE	1      /* 2x16 parts on daughter card */

#define DDR_100		100    /* type found on most mem d-boards */
#define DDR_111		111    /* some combo parts */
#define DDR_133		133    /* most combo, some mem d-boards */
#define DDR_165		165    /* future parts */

#define CPU_3430	0x3430
#define CPU_2430	0x2430
#define CPU_2420	0x2420
#define CPU_1710	0x1710
#define CPU_1610	0x1610

/**
 * CPU revision
 */
#define CPU_ES1		1
#define CPU_ES1P1	2
#define CPU_ES1P2	3
#define CPU_ES2		4
#define CPU_ES2P1	5
#define CPU_ES2P2	6
#define CPU_ES3		7
#define CPU_ES3P1	8
#define CPU_ES3P2	9
#define CPU_ES4		10
#define CPU_ES4P1	11
#define CPU_ES4P2	12

#define GPMC_MUXED		1
#define GPMC_NONMUXED		0

#define TYPE_NAND		0x800	/* bit pos for nand in gpmc reg */
#define TYPE_NOR		0x000
#define TYPE_ONENAND		0x800

#define WIDTH_8BIT		0x0000
#define WIDTH_16BIT		0x1000	/* bit pos for 16 bit in gpmc */

#define TST_DEVICE              0x0
#define EMU_DEVICE              0x1
#define HS_DEVICE               0x2
#define GP_DEVICE               0x3

/** These are implemented by the System specific code in omapX-generic.c */
u32 get_cpu_type(void);
u32 get_cpu_rev(void);
u32 get_sdr_cs_size(u32 offset);
inline u32 get_sysboot_value(void);
u32 get_gpmc0_base(void);
u32 get_base(void);
u32 running_in_flash(void);
u32 running_in_sram(void);
u32 running_in_sdram(void);
u32 get_boot_type(void);
u32 get_device_type(void);

#endif /*__ASM_ARCH_SYS_INFO_H_ */
