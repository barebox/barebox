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

/* #define DEBUG */

#define CONFIG_BOARDINFO "Phytec Phycore mpc5200b tiny"

/*------------------------------------------------------------------------------------------------------------------------------------------------------
High Level Configuration Options
(easy to change)
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_MPC5xxx		1	/* This is an MPC5xxx CPU */
#define CONFIG_MPC5200		1	/* (more precisely an MPC5200 CPU) */
#define CONFIG_MPC5200_DDR	1	/* (with DDR-SDRAM) */
#define CONFIG_PHYCORE_MPC5200B_TINY		1	/* ... on phyCORE-MPC5200B -> FEC configuration and IDE */
#define CFG_MPC5XXX_CLKIN	33333333 /* ... running at 33.333333MHz */
#define BOOTFLAG_COLD		0x01	/* Normal Power-On: Boot from FLASH  */
#define BOOTFLAG_WARM		0x02	/* Software reboot	     */
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#  define CFG_CACHELINE_SHIFT	5	/* log base 2 of the above value */
#endif

/*------------------------------------------------------------------------------------------------------------------------------------------------------
Serial console configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_BAUDRATE		115200	/* ... at 115200 bps */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200, 230400 }

#if (TEXT_BASE == 0xFF000000)		/* Boot low */
#define CFG_LOWBOOT		1
#endif
/* RAMBOOT will be defined automatically in memory section */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
Autobooting
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_BOOTDELAY	5   /* autoboot after 5 seconds */

#define CONFIG_PREBOOT	"echo;"	\
	"echo Type \"run net_nfs\" to load Kernel over TFTP and to mount root filesystem over NFS;" \
	"echo"

#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"netdev=eth0\0"							\
	"nfsargs=setenv bootargs root=/dev/nfs rw "			\
		"nfsroot=$(serverip):$(rootpath)\0"			\
	"ramargs=setenv bootargs root=/dev/ram rw\0"			\
	"addip=setenv bootargs $(bootargs) "				\
		"ip=$(ipaddr):$(serverip):$(gatewayip):$(netmask)"	\
		":$(hostname):$(netdev):off panic=5\0"			\
	"flash_nfs=run nfsargs addip;"					\
		"bootm $(kernel_addr)\0"				\
	"flash_self=run ramargs addip;"					\
		"bootm $(kernel_addr) $(ramdisk_addr)\0"		\
	"net_nfs=tftp 200000 $(bootfile);run nfsargs addip;bootm\0"	\
	"rootpath=/nfs/rootfs\0"					\
	"bootfile=/phycore_mpc5200b_tiny/uImage\0"						\
	"ipaddr=172.16.2.37\0"						\
	"netmask=255.255.0.0\0"						\
	"serverip=172.16.2.34\0"					\
	"update=tftpboot 400000 u-boot-phycore_mpc5200b_tiny.bin;erase 1:120-123;cp.b 400000 FFF00000 80000\0"	\
	""
/*	"lowup=tftpboot 400000 u-boot-low.bin;erase 1:0-1;cp.b 400000 FF000000 80000\0"	\ */
#define CONFIG_BOOTCOMMAND	"run net_nfs"

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
/*------------------------------------------------------------------------------------------------------------------------------------------------------
 I2C configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_HARD_I2C		1	/* I2C with hardware support */
#define CFG_I2C_MODULE		2	/* Select I2C module #1 or #2 */
#define CFG_I2C_SPEED		100000 	/* 100 kHz */
#define CFG_I2C_SLAVE		0x7F

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 EEPROM CAT24WC32 configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_I2C_EEPROM_ADDR		0x52	/* 1010100x */
#define CFG_I2C_FACT_ADDR		0x52	/* EEPROM CAT24WC32 */
#define CFG_I2C_EEPROM_ADDR_LEN		2	/* Bytes of address */
#define CFG_EEPROM_SIZE			2048
#define CFG_EEPROM_PAGE_WRITE_BITS	3
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS	15

/*------------------------------------------------------------------------------------------------------------------------------------------------------
RTC configuration
------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define RTC
#define CONFIG_RTC_PCF8563	1
#define CFG_I2C_RTC_ADDR	0x51

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 Flash configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_FLASH_BASE		0xFF000000
#define CFG_FLASH_CFI		1	/* Flash is CFI conformant */
#define CFG_FLASH_CFI_DRIVER	1	/* Use the common driver */
//#define CFG_FLASH_BANKS_LIST	{ CFG_BOOTCS_START }
#define CFG_FLASH_BANKS_LIST	{ CFG_FLASH_BASE }
#define CFG_FLASH_EMPTY_INFO
#define CFG_FLASH_SIZE		0x01000000 /* 16 MByte */
#define CFG_MAX_FLASH_SECT	128	/* max num of sects on one chip */
#define CFG_MAX_FLASH_BANKS	1	 /* max num of flash banks (= chip selects) */
#define CFG_FLASH_USE_BUFFER_WRITE	/* not supported yet for AMD */
/* we only use CS-Boot */
#define CFG_BOOTCS_START	CFG_FLASH_BASE
#define CFG_BOOTCS_SIZE		CFG_FLASH_SIZE

#define PHYCORE_MPC5200B_TINY_REV 0

#if PHYCORE_MPC5200B_TINY_REV == 0
#define CFG_BOOTCS_CFG		0x00083800
#elif PHYCORE_MPC5200B_TINY_REV == 1
#define CFG_BOOTCS_CFG		0x0008FD00
#else
#error "PHYCORE_MPC5200B_TINY_REV not specified"
#endif

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 Environment settings
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* #define CFG_ENV_IS_IN_FLASH	1 */
#define CFG_ENV_IS_IN_EEPROM	1
#define CFG_ENV_OFFSET		0x00			/* environment starts at the beginning of the EEPROM */
#define CFG_ENV_SIZE		2048
/* #define CFG_ENV_SECT_SIZE	0x10000 */
#define CONFIG_ENV_OVERWRITE	1

/*------------------------------------------------------------------------------------------------------------------------------------------------------
  Memory map
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_MBAR		0xF0000000 /* MBAR hast to be switched by other bootloader or debugger config  */
#define CFG_SDRAM_BASE		0x00000000
//#define CFG_DEFAULT_MBAR	0xF0000000
/* Use SRAM until RAM will be available */
#define CFG_INIT_RAM_ADDR	MPC5XXX_SRAM
#define CFG_INIT_RAM_END	MPC5XXX_SRAM_SIZE	/* End of used area in DPRAM */
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

#define CFG_MONITOR_BASE    TEXT_BASE
#if (CFG_MONITOR_BASE < CFG_FLASH_BASE)
#   define CFG_RAMBOOT		1
#endif

#define CFG_MONITOR_LEN		(192 << 10)	/* Reserve 192 kB for Monitor	*/
#define CFG_MALLOC_LEN		(2 << 20)	/* Reserve 2 MB for malloc()	*/
#define CFG_BOOTMAPSZ		(8 << 20)	/* Initial Memory map for Linux */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 GPIO configuration
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* #define CFG_GPS_PORT_CONFIG	0x00551c12 */
#define CFG_GPS_PORT_CONFIG	0x00558c10 /* PSC6=UART, PSC3=UART ; Ether=100MBit with MD */

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 Miscellaneous configurable options
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_LONGHELP			/* undef to save memory	    */
#define CFG_CBSIZE		1024	/* Console I/O Buffer Size  */
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CONFIG_PROMPT)+16)	/* Print Buffer Size */
#define CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size	*/

#define CFG_MEMTEST_START	0x00100000	/* memtest works on */
#define CFG_MEMTEST_END		0x00f00000	/* 1 ... 15 MB in DRAM	*/

#define CFG_LOAD_ADDR		0x100000	/* default load address */
#define CFG_HZ			1000	/* decrementer freq: 1 ms ticks */

#define CONFIG_DISPLAY_BOARDINFO 1
#define CONFIG_CMDLINE_EDITING  1

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 Various low-level settings
 ------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CFG_HID0_INIT		HID0_ICE | HID0_ICFI
#define CFG_HID0_FINAL		HID0_ICE

#define CFG_CS_BURST		0x00000000
#define CFG_CS_DEADCYCLE	0x33333333
#define CFG_RESET_ADDRESS	0xff000000


/* pass open firmware flat tree */
//#define CONFIG_OF_FLAT_TREE	1
//#define CONFIG_OF_BOARD_SETUP	1

#define OF_CPU			"PowerPC,5200@0"
#define OF_TBCLK		CFG_MPC5XXX_CLKIN
#define OF_SOC                  "soc5200@f0000000"

#endif /* __CONFIG_H */
