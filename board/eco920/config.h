/*
 * (C) Copyright 2007 Pengutronix
 * Sascha Hauer, <s.hauer@pengutronix.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* ARM asynchronous clock */
#define AT91C_MAIN_CLOCK	179712000	/* from 18.432 MHz crystal (18432000 / 4 * 39) */
#define AT91C_MASTER_CLOCK	59904000	/* peripheral clock (AT91C_MASTER_CLOCK / 3) */
/* #define AT91C_MASTER_CLOCK	44928000 */	/* peripheral clock (AT91C_MASTER_CLOCK / 4) */

#define AT91_SLOW_CLOCK		32768	/* slow clock */

#undef  CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff	*/
#define USE_920T_MMU		1

#define CONFIG_CMDLINE_TAG	1	/* enable passing of ATAGs	*/
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG	1

#define CFG_USE_MAIN_OSCILLATOR		1
/* flash */
#define MC_PUIA_VAL	0x00000000
#define MC_PUP_VAL	0x00000000
#define MC_PUER_VAL	0x00000000
#define MC_ASR_VAL	0x00000000
#define MC_AASR_VAL	0x00000000
#define EBI_CFGR_VAL	0x00000000
#define SMC2_CSR_VAL	0x00003287

/* clocks */
#define PLLAR_VAL	0x2026be04
#define PLLBR_VAL	0x10483e0e
#define MCKR_VAL	0x00000202

/* sdram */
#define PIOC_ASR_VAL	0xffff0000
#define PIOC_BSR_VAL	0x00000000
#define PIOC_PDR_VAL	0xffff0000
#define EBI_CSA_VAL	0x00000002 /* CS1=SDRAM */
#define SDRC_CR_VAL	0x2188c155 /* set up the SDRAM */
#define SDRAM		0x20000000 /* address of the SDRAM */
#define SDRAM1		0x20000080 /* address of the SDRAM */
#define SDRAM_VAL	0x00000000 /* value written to SDRAM */
#define SDRC_MR_VAL	0x00000002 /* Precharge All */
#define SDRC_MR_VAL1	0x00000004 /* refresh */
#define SDRC_MR_VAL2	0x00000003 /* Load Mode Register */
#define SDRC_MR_VAL3	0x00000000 /* Normal Mode */
#define SDRC_TR_VAL	0x000002E0 /* Write refresh rate */

#define CONFIG_BAUDRATE 115200

/*
 * Hardware drivers
 */

/* define one of CONFIG_DBGU, CONFIG_USART0 or CONFIG_USART1 to choose console */
#define CONFIG_DBGU

#define CONFIG_BOOTDELAY      3

#define CONFIG_CMDLINE_EDITING  1

#define	CONFIG_EXTRA_ENV_SETTINGS											\
	"mtdids=nor0=physmap-flash.0\0"											\
	"mtdparts=mtdparts=physmap-flash.0:128k(barebox)ro,128k(env),1536k(kernel),-(jffs2)\0"				\
	"bootargs_base=setenv bootargs console=ttyAT0,115200\0"								\
	"bootargs_nfs=setenv bootargs $(bootargs) root=/dev/nfs ip=dhcp nfsroot=$(serverip):$(nfsrootfs),v3,tcp\0"	\
	"bootargs_mtd=setenv bootargs $(bootargs) $(mtdparts)\0"							\
	"bootargs_flash=setenv bootargs $(bootargs) root=/dev/mtdblock3 rootfstype=jffs2\0"				\
	"bootcmd_flash=run bootargs_base bootargs_mtd bootargs_flash; bootm 0x11040000\0"				\
	"bootcmd_net=run bootargs_base bootargs_mtd bootargs_nfs; tftpboot 0x20000000 $(uimage); bootm\0"		\
	"autoload=n\0"													\
	"uimage=uImage-eco920\0"											\
	"jffs2=root-eco920.jffs2\0"

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM			0x20000000
#define PHYS_SDRAM_SIZE			0x2000000

#define CFG_MEMTEST_START		PHYS_SDRAM
#define CFG_MEMTEST_END			CFG_MEMTEST_START + PHYS_SDRAM_SIZE - 262144

#define CONFIG_DRIVER_ETHER
#define CONFIG_NET_RETRY_COUNT		20
#define CONFIG_AT91C_USE_RMII

#define CFG_LOAD_ADDR		0x21000000  /* default load address */

#define CFG_BAUDRATE_TABLE	{115200 , 19200, 38400, 57600, 9600 }

#define	CFG_LONGHELP				/* undef to save memory		*/
#define CFG_PROMPT		"barebox> "	/* Monitor Command Prompt */
#define CFG_CBSIZE		1024		/* Console I/O Buffer Size */
#define CFG_MAXARGS		32		/* max number of command args */
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */

#define CLOCK_TICK_RATE		AT91C_MASTER_CLOCK/2	/* AT91C_TC0_CMR is implicitly set to */
							/* AT91C_TC_TIMER_DIV1_CLOCK */

#define CONFIG_MISC_INIT_R      1       /* call misc_init_r() on init   */
#define CFG_SPLASH		1
#define CFG_S1D13706FB		1

#define CFG_USB_OHCI_MAX_ROOT_PORTS     15
#define CFG_USB_OHCI_SLOT_NAME           "at91rm9200"
#define LITTLEENDIAN
#define CONFIG_AT91C_PQFP_UHPBUG

#endif

