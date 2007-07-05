/*
 * Copyright (C) 2003 ETC s.r.o.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Written by Peter Figuli <peposh@etc.sk>, 2003.
 *
 * 2003/13/06 Initial MP10 Support copied from wepep250
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_IMX_SERIAL1

#define CONFIG_ARCH_NUMBER MACH_TYPE_SCB9328
#define CONFIG_BOOT_PARAMS 0x08000100

/*
 * General options for u-boot. Modify to save memory foot print
 */
#define CFG_PBSIZE (CONFIG_CBSIZE+sizeof(CONFIG_PROMPT)+16) /* print buffer size   */

#define CFG_CPUSPEED		0x141	     /* core clock - register value  */

/*
 * Definitions related to passing arguments to kernel.
 */

#define CFG_MALLOC_LEN		(4096 << 10)

#define CONFIG_STACKSIZE	(120<<10)      /* stack size		     */


/* CNC == 3 too long
   #define CFG_CS5U_VAL 0x0000C210 */

/* #define CFG_CS5U_VAL 0x00008400
   mal laenger mahcen, ob der bei 150MHz laenger haelt dann und
   kaum langsamer ist */
/* #define CFG_CS5U_VAL 0x00009400
   #define CFG_CS5L_VAL 0x11010D03 */

#define CONFIG_DM9000_BASE		0x16000000
#define DM9000_IO			CONFIG_DM9000_BASE
#define DM9000_DATA			(CONFIG_DM9000_BASE+4)
/* #define CONFIG_DM9000_USE_8BIT */
#define CONFIG_DM9000_USE_16BIT
/* #define CONFIG_DM9000_USE_32BIT */

/* f_{dpll}=2*f{ref}*(MFI+MFN/(MFD+1))/(PD+1)
   f_ref=16,777MHz

   0x002a141f: 191,9944MHz
   0x040b2007: 144MHz
   0x042a141f: 96MHz
   0x0811140d: 64MHz
   0x040e200e: 150MHz
   0x00321431: 200MHz

   0x08001800: 64MHz mit 16er Quarz
   0x04001800: 96MHz mit 16er Quarz
   0x04002400: 144MHz mit 16er Quarz

   31 |x x x x|x x x x|x x x x|x x x x|x x x x|x x x x|x x x x|x x x x| 0
      |XXX|--PD---|-------MFD---------|XXX|--MFI--|-----MFN-----------|	    */

#define CPU200

#ifdef CPU200
#define CFG_MPCTL0_VAL 0x00321431
#else
#define CFG_MPCTL0_VAL 0x040e200e
#endif

/* #define BUS64 */
#define BUS72

#ifdef BUS72
#define CFG_SPCTL0_VAL 0x04002400
#endif

#ifdef BUS96
#define CFG_SPCTL0_VAL 0x04001800
#endif

#ifdef BUS64
#define CFG_SPCTL0_VAL 0x08001800
#endif

/* Das ist der BCLK Divider, der aus der System PLL
   BCLK und HCLK erzeugt:
   31 | xxxx xxxx xxxx xxxx xx10 11xx xxxx xxxx | 0
   0x2f008403 : 192MHz/2=96MHz, 144MHz/2=72MHz PRESC=1->BCLKDIV=2
   0x2f008803 : 192MHz/3=64MHz, 240MHz/3=80MHz PRESC=1->BCLKDIV=2
   0x2f001003 : 192MHz/5=38,4MHz
   0x2f000003 : 64MHz/1
   Bit 22: SPLL Restart
   Bit 21: MPLL Restart */

#ifdef BUS64
#define CFG_CSCR_VAL 0x2f030003
#endif

#ifdef BUS72
#define CFG_CSCR_VAL 0x2f030403
#endif

#define MHZ16QUARZINUSE

#ifdef MHZ16QUARZINUSE
#define CONFIG_SYSPLL_CLK_FREQ 16000000
#else
#define CONFIG_SYSPLL_CLK_FREQ 16780000
#endif

#define CONFIG_SYS_CLK_FREQ 16780000

/* FMCR Bit 0 becomes 0 to make CS3 CS3 :P */
#define CFG_FMCR_VAL 0x00000001

/* Bit[0:3] contain PERCLK1DIV for UART 1
   0x000b00b ->b<- -> 192MHz/12=16MHz
   0x000b00b ->8<- -> 144MHz/09=16MHz
   0x000b00b ->3<- -> 64MHz/4=16MHz */

#ifdef BUS96
#define CFG_PCDR_VAL 0x000b00b5
#endif

#ifdef BUS64
#define CFG_PCDR_VAL 0x000b00b3
#endif

#ifdef BUS72
#define CFG_PCDR_VAL 0x000b00b8
#endif

#endif	/* __CONFIG_H */
