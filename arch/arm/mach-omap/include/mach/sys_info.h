/**
 * @file
 * @brief This file defines the macros apis which are useful for most OMAP
 * platforms.
 *
 * These are implemented by the System specific code in omapX-generic.c
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 *
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

#define CPU_1610	0x1610
#define CPU_1710	0x1710
#define CPU_2420	0x2420
#define CPU_2430	0x2430
#define CPU_3350	0x3350
#define CPU_3430	0x3430
#define CPU_3630	0x3630
#define CPU_AM35XX	0x3500

/**
 * Define CPU revisions
 */
#define	cpu_revision(cpu,rev)	(((cpu) << 16) | (rev))

#define OMAP34XX_ES1		cpu_revision(CPU_3430, 0)
#define OMAP34XX_ES2		cpu_revision(CPU_3430, 1)
#define OMAP34XX_ES2_1		cpu_revision(CPU_3430, 2)
#define OMAP34XX_ES3		cpu_revision(CPU_3430, 3)
#define OMAP34XX_ES3_1		cpu_revision(CPU_3430, 4)

#define AM335X_ES1_0		cpu_revision(CPU_3350, 0)
#define AM335X_ES2_0		cpu_revision(CPU_3350, 1)
#define AM335X_ES2_1		cpu_revision(CPU_3350, 2)

#define OMAP36XX_ES1		cpu_revision(CPU_3630, 0)
#define OMAP36XX_ES1_1		cpu_revision(CPU_3630, 1)
#define OMAP36XX_ES1_2		cpu_revision(CPU_3630, 2)

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

/**
 * Hawkeye definitions to identify silicon families
 */
#define OMAP_HAWKEYE_34XX	0xB7AE /* OMAP34xx */
#define OMAP_HAWKEYE_36XX	0xB891 /* OMAP36xx */
#define OMAP_HAWKEYE_335X	0xB944 /* AM335x */
#define OMAP_HAWKEYE_AM35XX	0xb868 /* AM35xx */

/** These are implemented by the System specific code in omapX-generic.c */
u32 get_cpu_type(void);
u32 get_cpu_rev(void);
u32 get_sdr_cs_size(u32 offset);
u32 get_sdr_cs1_base(void);
u32 get_sysboot_value(void);
u32 get_boot_type(void);
u32 get_device_type(void);

#endif /*__ASM_ARCH_SYS_INFO_H_ */
