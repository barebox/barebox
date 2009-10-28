/*
 * (C) Copyright 2003-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2006
 * Eric Schumann, Phytec Messatechnik GmbH
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <mach/mpc5xxx.h>

/* #define DEBUG */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
High Level Configuration Options
(easy to change)
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_MPC5200_DDR	1	/* (with DDR-SDRAM) */
#define CFG_MPC5XXX_CLKIN	33333333 /* ... running at 33.333333MHz */
#define BOOTFLAG_COLD		0x01	/* Normal Power-On: Boot from FLASH  */
#define BOOTFLAG_WARM		0x02	/* Software reboot	     */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
Serial console configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/

#if (TEXT_BASE == 0xFF000000)		/* Boot low */
#define CFG_LOWBOOT		1
#endif
/* RAMBOOT will be defined automatically in memory section */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
IPB Bus clocking configuration.
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define  CFG_IPBSPEED_133		/* define for 133MHz speed */
#if defined(CFG_IPBSPEED_133)
/*
 * PCI Bus clocking configuration
 *
 * Actually a PCI Clock of 66 MHz is only set (in cpu_init.c) if
 * CFG_IPBSPEED_133 is defined. This is because a PCI Clock of 66 MHz yet hasn't
 * been tested with a IPB Bus Clock of 66 MHz.
 */
#define CFG_PCISPEED_66		/* define for 66MHz speed */
#else
#undef CFG_PCISPEED_66			/* for 33MHz speed */
#endif

/* we only use CS-Boot */
#define CFG_BOOTCS_START	0xFF000000
#define CFG_BOOTCS_SIZE		0x01000000

#if CONFIG_MACH_PHYCORE_MPC5200B_TINY_REV == 1
#define CFG_BOOTCS_CFG		0x0008FD00
#else
#define CFG_BOOTCS_CFG		0x00083800
#endif

/*------------------------------------------------------------------------------------------------------------------------------------------------------
  Memory map
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_MBAR		0xF0000000 /* MBAR hast to be switched by other bootloader or debugger config  */
#define CFG_SDRAM_BASE		0x00000000

/* Use SRAM until RAM will be available */
#define CFG_INIT_RAM_ADDR	MPC5XXX_SRAM
#define CFG_INIT_RAM_SIZE	MPC5XXX_SRAM_SIZE	/* End of used area in DPRAM */
#define CONFIG_EARLY_INITDATA_SIZE 0x100

#define CFG_BOOTMAPSZ		(8 << 20)	/* Initial Memory map for Linux */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 GPIO configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_GPS_PORT_CONFIG	0x00558c10 /* PSC6=UART, PSC3=UART ; Ether=100MBit with MD */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 Various low-level settings
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_HID0_INIT		HID0_ICE | HID0_ICFI
#define CFG_HID0_FINAL		HID0_ICE

#define CFG_CS_BURST		0x00000000
#define CFG_CS_DEADCYCLE	0x33333333

#define OF_CPU			"PowerPC,5200@0"
#define OF_TBCLK		CFG_MPC5XXX_CLKIN
#define OF_SOC                  "soc5200@f0000000"

#endif /* __CONFIG_H */
