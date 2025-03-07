// SPDX-License-Identifier: BSD-1-Clause
/*
 * Chip-specific header file for the SAMA5D2 family
 *
 * Copyright (c) 2015, Atmel Corporation
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 *
 * Common definitions.
 * Based on SAMA5D2 datasheet:
 * http://ww1.microchip.com/downloads/en/DeviceDoc/SAMA5D2-Series-Data-Sheet-DS60001476C.pdf
 *
 */

#ifndef SAMA5D2_H
#define SAMA5D2_H

#include <asm/io.h>
#include <linux/sizes.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>

/*
 * Peripheral identifiers/interrupts. (Table 18-9)
 */
#define	SAMA5D2_ID_FIQ		0	/* FIQ Interrupt ID */
/* 1 */
#define	SAMA5D2_ID_ARM		2	/* Performance Monitor Unit */
#define	SAMA5D2_ID_PIT		3	/* Periodic Interval Timer Interrupt */
#define	SAMA5D2_ID_WDT		4	/* Watchdog Timer Interrupt */
#define	SAMA5D2_ID_GMAC		5	/* Ethernet MAC */
#define	SAMA5D2_ID_XDMAC0	6	/* DMA Controller 0 */
#define	SAMA5D2_ID_XDMAC1	7	/* DMA Controller 1 */
#define	SAMA5D2_ID_ICM		8	/* Integrity Check Monitor */
#define	SAMA5D2_ID_AES		9	/* Advanced Encryption Standard */
#define	SAMA5D2_ID_AESB		10	/* AES bridge */
#define	SAMA5D2_ID_TDES		11	/* Triple Data Encryption Standard */
#define	SAMA5D2_ID_SHA		12	/* SHA Signature */
#define	SAMA5D2_ID_MPDDRC	13	/* MPDDR Controller */
#define	SAMA5D2_ID_MATRIX1	14	/* H32MX, 32-bit AHB Matrix */
#define	SAMA5D2_ID_MATRIX0	15	/* H64MX, 64-bit AHB Matrix */
#define	SAMA5D2_ID_SECUMOD	16	/* Secure Module */
#define	SAMA5D2_ID_HSMC		17	/* Multi-bit ECC interrupt */
#define	SAMA5D2_ID_PIOA		18	/* Parallel I/O Controller A */
#define	SAMA5D2_ID_FLEXCOM0	19	/* FLEXCOM0 */
#define	SAMA5D2_ID_FLEXCOM1	20	/* FLEXCOM1 */
#define	SAMA5D2_ID_FLEXCOM2	21	/* FLEXCOM2 */
#define	SAMA5D2_ID_FLEXCOM3	22	/* FLEXCOM3 */
#define	SAMA5D2_ID_FLEXCOM4	23	/* FLEXCOM4 */
#define	SAMA5D2_ID_UART0	24	/* UART0 */
#define	SAMA5D2_ID_UART1	25	/* UART1 */
#define	SAMA5D2_ID_UART2	26	/* UART2 */
#define	SAMA5D2_ID_UART3	27	/* UART3 */
#define	SAMA5D2_ID_UART4	28	/* UART4 */
#define	SAMA5D2_ID_TWI0		29	/* Two-wire Interface 0 */
#define	SAMA5D2_ID_TWI1		30	/* Two-wire Interface 1 */
#define	SAMA5D2_ID_SDMMC0	31	/* Secure Data Memory Card Controller 0 */
#define	SAMA5D2_ID_SDMMC1	32	/* Secure Data Memory Card Controller 1 */
#define	SAMA5D2_ID_SPI0		33	/* Serial Peripheral Interface 0 */
#define	SAMA5D2_ID_SPI1		34	/* Serial Peripheral Interface 1 */
#define	SAMA5D2_ID_TC0		35	/* Timer Counter 0 (ch.0,1,2) */
#define	SAMA5D2_ID_TC1		36	/* Timer Counter 1 (ch.3,4,5) */
/* 37 */
#define	SAMA5D2_ID_PWM		38	/* Pulse Width Modulation Controller0 (ch. 0,1,2,3) */
/* 39 */
#define	SAMA5D2_ID_ADC		40	/* Touch Screen ADC Controller */
#define	SAMA5D2_ID_UHPHS	41	/* USB Host High Speed */
#define	SAMA5D2_ID_UDPHS	42	/* USB Device High Speed */
#define	SAMA5D2_ID_SSC0		43	/* Serial Synchronous Controller 0 */
#define	SAMA5D2_ID_SSC1		44	/* Serial Synchronous Controller 1 */
#define	SAMA5D2_ID_LCDC		45	/* LCD Controller */
#define	SAMA5D2_ID_ISI		46	/* Image Sensor Interface */
#define	SAMA5D2_ID_TRNG		47	/* True Random Number Generator */
#define	SAMA5D2_ID_PDMIC	48	/* Pulse Density Modulation Interface Controller */
#define	SAMA5D2_ID_IRQ		49	/* IRQ Interrupt ID */
#define	SAMA5D2_ID_SFC		50	/* Fuse Controller */
#define	SAMA5D2_ID_SECURAM	51	/* Secure RAM */
#define	SAMA5D2_ID_QSPI0	52	/* QSPI0 */
#define	SAMA5D2_ID_QSPI1	53	/* QSPI1 */
#define	SAMA5D2_ID_I2SC0	54	/* Inter-IC Sound Controller 0 */
#define	SAMA5D2_ID_I2SC1	55	/* Inter-IC Sound Controller 1 */
#define	SAMA5D2_ID_CAN0_INT0	56	/* MCAN 0 Interrupt0 */
#define	SAMA5D2_ID_CAN1_INT0	57	/* MCAN 1 Interrupt0 */
#define	SAMA5D2_ID_PTC		58	/* Peripheral Touch Controller */
#define	SAMA5D2_ID_CLASSD	59	/* Audio Class D Amplifier */
#define	SAMA5D2_ID_SFR		60	/* Special Function Register */
#define	SAMA5D2_ID_SAIC		61	/* Secured Advanced Interrupt Controller */
#define	SAMA5D2_ID_AIC		62	/* Advanced Interrupt Controller */
#define	SAMA5D2_ID_L2CC		63	/* L2 Cache Controller */
#define	SAMA5D2_ID_CAN0_INT1	64	/* MCAN 0 Interrupt1 */
#define	SAMA5D2_ID_CAN1_INT1	65	/* MCAN 1 Interrupt1 */
#define	SAMA5D2_ID_GMAC_Q1	66	/* GMAC Queue 1 Interrupt */
#define	SAMA5D2_ID_GMAC_Q2	67	/* GMAC Queue 2 Interrupt */
#define	SAMA5D2_ID_PIOB		68	/* Parallel I/O Controller B */
#define	SAMA5D2_ID_PIOC		69	/* Parallel I/O Controller C */
#define	SAMA5D2_ID_PIOD		70	/* Parallel I/O Controller D */
#define	SAMA5D2_ID_SDMMC0_TIMER	71	/* Secure Data Memory Card Controller 0 */
#define	SAMA5D2_ID_SDMMC1_TIMER	72	/* Secure Data Memory Card Controller 1 */
/* 73 */
#define	SAMA5D2_ID_SYS		74	/* System Controller Interrupt */
#define	SAMA5D2_ID_ACC		75	/* Analog Comparator */
#define	SAMA5D2_ID_RXLP		76	/* UART Low-Power */
#define	SAMA5D2_ID_SFRBU	77	/* Special Function Register BackUp */
#define	SAMA5D2_ID_CHIPID	78	/* Chip ID */

/*
 * User Peripheral physical base addresses.
 */

#define	SAMA5D2_BASE_LCDC	IOMEM(0xf0000000)
#define	SAMA5D2_BASE_XDMAC1	IOMEM(0xf0004000)
#define	SAMA5D2_BASE_HXISI	IOMEM(0xf0008000)
#define	SAMA5D2_BASE_MPDDRC	IOMEM(0xf000c000)
#define	SAMA5D2_BASE_XDMAC0	IOMEM(0xf0010000)
#define	SAMA5D2_BASE_PMC	IOMEM(0xf0014000)
#define	SAMA5D2_BASE_MATRIX64	IOMEM(0xf0018000)	/* MATRIX0 */
#define	SAMA5D2_BASE_AESB	IOMEM(0xf001c000)
#define	SAMA5D2_BASE_QSPI0	IOMEM(0xf0020000)
#define	SAMA5D2_BASE_QSPI1	IOMEM(0xf0024000)
#define	SAMA5D2_BASE_SHA	IOMEM(0xf0028000)
#define	SAMA5D2_BASE_AES	IOMEM(0xf002c000)

#define	SAMA5D2_BASE_SPI0	IOMEM(0xf8000000)
#define	SAMA5D2_BASE_SSC0	IOMEM(0xf8004000)
#define	SAMA5D2_BASE_GMAC	IOMEM(0xf8008000)
#define	SAMA5D2_BASE_TC0	IOMEM(0xf800c000)
#define	SAMA5D2_BASE_TC1	IOMEM(0xf8010000)
#define	SAMA5D2_BASE_HSMC	IOMEM(0xf8014000)
#define	SAMA5D2_BASE_PDMIC	IOMEM(0xf8018000)
#define	SAMA5D2_BASE_UART0	IOMEM(0xf801c000)
#define	SAMA5D2_BASE_UART1	IOMEM(0xf8020000)
#define	SAMA5D2_BASE_UART2	IOMEM(0xf8024000)
#define	SAMA5D2_BASE_TWI0	IOMEM(0xf8028000)
#define	SAMA5D2_BASE_PWMC	IOMEM(0xf802c000)
#define	SAMA5D2_BASE_SFR	IOMEM(0xf8030000)
#define	SAMA5D2_BASE_FLEXCOM0	IOMEM(0xf8034000)
#define	SAMA5D2_BASE_FLEXCOM1	IOMEM(0xf8038000)
#define	SAMA5D2_BASE_SAIC	IOMEM(0xf803c000)
#define	SAMA5D2_BASE_ICM	IOMEM(0xf8040000)
#define	SAMA5D2_BASE_SECURAM	IOMEM(0xf8044000)
#define	SAMA5D2_BASE_SYSC	IOMEM(0xf8048000)
#define	SAMA5D2_BASE_ACC	IOMEM(0xf804a000)
#define	SAMA5D2_BASE_SFC	IOMEM(0xf804c000)
#define	SAMA5D2_BASE_I2SC0	IOMEM(0xf8050000)
#define	SAMA5D2_BASE_CAN0	IOMEM(0xf8054000)

#define	SAMA5D2_BASE_SPI1	IOMEM(0xfc000000)
#define	SAMA5D2_BASE_SSC1	IOMEM(0xfc004000)
#define	SAMA5D2_BASE_UART3	IOMEM(0xfc008000)
#define	SAMA5D2_BASE_UART4	IOMEM(0xfc00c000)
#define	SAMA5D2_BASE_FLEXCOM2	IOMEM(0xfc010000)
#define	SAMA5D2_BASE_FLEXCOM3	IOMEM(0xfc014000)
#define	SAMA5D2_BASE_FLEXCOM4	IOMEM(0xfc018000)
#define	SAMA5D2_BASE_TRNG	IOMEM(0xfc01c000)
#define	SAMA5D2_BASE_AIC	IOMEM(0xfc020000)
#define	SAMA5D2_BASE_TWI1	IOMEM(0xfc028000)
#define	SAMA5D2_BASE_UDPHS	IOMEM(0xfc02c000)
#define	SAMA5D2_BASE_ADC	IOMEM(0xfc030000)

#define	SAMA5D2_BASE_PIOA	IOMEM(0xfc038000)
#define	SAMA5D2_BASE_MATRIX32	IOMEM(0xfc03c000)	/* MATRIX1 */
#define	SAMA5D2_BASE_SECUMOD	IOMEM(0xfc040000)
#define	SAMA5D2_BASE_TDES	IOMEM(0xfc044000)
#define	SAMA5D2_BASE_CLASSD	IOMEM(0xfc048000)
#define	SAMA5D2_BASE_I2SC1	IOMEM(0xfc04c000)
#define	SAMA5D2_BASE_CAN1	IOMEM(0xfc050000)
#define	SAMA5D2_BASE_SFRBU	IOMEM(0xfc05c000)
#define	SAMA5D2_BASE_CHIPID	IOMEM(0xfc069000)

/*
 * Address Memory Space
 */
#define	SAMA5D2_BASE_INTERNAL_MEM	IOMEM(0x00000000)
#define	SAMA5D2_BASE_CS0		IOMEM(0x10000000)
#define	SAMA5D2_BASE_DDRCS		IOMEM(0x20000000)
#define	SAMA5D2_BASE_DDRCS_AES		IOMEM(0x40000000)
#define	SAMA5D2_BASE_CS1		IOMEM(0x60000000)
#define	SAMA5D2_BASE_CS2		IOMEM(0x70000000)
#define	SAMA5D2_BASE_CS3		IOMEM(0x80000000)
#define	SAMA5D2_BASE_QSPI0_AES_MEM	IOMEM(0x90000000)
#define	SAMA5D2_BASE_QSPI1_AES_MEM	IOMEM(0x98000000)
#define	SAMA5D2_BASE_SDHC0		IOMEM(0xa0000000)
#define	SAMA5D2_BASE_SDHC1		IOMEM(0xb0000000)
#define	SAMA5D2_BASE_NFC_CMD_REG	IOMEM(0xc0000000)
#define	SAMA5D2_BASE_QSPI0_MEM		IOMEM(0xd0000000)
#define	SAMA5D2_BASE_QSPI1_MEM		IOMEM(0xd8000000)
#define	SAMA5D2_BASE_PERIPH		IOMEM(0xf0000000)

/*
 * Internal Memories
 */
#define	SAMA5D2_BASE_ROM		IOMEM(0x00000000)	/* ROM */
#define	SAMA5D2_BASE_ECC_ROM		IOMEM(0x00060000)	/* ECC ROM */
#define	SAMA5D2_BASE_NFC_SRAM		0x00100000		/* NFC SRAM */
#define	SAMA5D2_BASE_SRAM0		0x00200000		/* SRAM0 */
#define	SAMA5D2_BASE_SRAM1		0x00220000		/* SRAM1 */
#define	SAMA5D2_BASE_UDPHS_SRAM		0x00300000		/* UDPHS RAM */
#define	SAMA5D2_BASE_UHP_OHCI		IOMEM(0x00400000)	/* UHP OHCI */
#define	SAMA5D2_BASE_UHP_EHCI		IOMEM(0x00500000)	/* UHP EHCI */
#define	SAMA5D2_BASE_AXI_MATRIX		IOMEM(0x00600000)	/* AXI Maxtrix */
#define	SAMA5D2_BASE_DAP		IOMEM(0x00700000)	/* DAP */
#define	SAMA5D2_BASE_PTC		IOMEM(0x00800000)	/* PTC */
#define	SAMA5D2_BASE_L2CC		IOMEM(0x00A00000)	/* L2CC */

/*
 * Other misc defines
 */
#define	SAMA5D2_BASE_PMECC	(SAMA5D2_BASE_HSMC + 0x70)
#define	SAMA5D2_BASE_PMERRLOC	(SAMA5D2_BASE_HSMC + 0x500)

#define	SAMA5D2_PMECC		(SAMA5D2_BASE_PMECC - SAMA5D2_BASE_SYS)
#define	SAMA5D2_PMERRLOC	(SAMA5D2_BASE_PMERRLOC - SAMA5D2_BASE_SYS)

#define	SAMA5D2_BASE_PIOB	(SAMA5D2_BASE_PIOA + 0x40)
#define	SAMA5D2_BASE_PIOC	(SAMA5D2_BASE_PIOB + 0x40)
#define	SAMA5D2_BASE_PIOD	(SAMA5D2_BASE_PIOC + 0x40)

/* SYSC spawns */
#define	SAMA5D2_BASE_RSTC	SAMA5D2_BASE_SYSC
#define	SAMA5D2_BASE_SHDC	(SAMA5D2_BASE_SYSC + 0x10)
#define	SAMA5D2_BASE_PITC	(SAMA5D2_BASE_SYSC + 0x30)
#define	SAMA5D2_BASE_WDT	(SAMA5D2_BASE_SYSC + 0x40)
#define	SAMA5D2_BASE_SCKCR	(SAMA5D2_BASE_SYSC + 0x50)
#define	SAMA5D2_BASE_RTCC	(SAMA5D2_BASE_SYSC + 0xb0)

#define SAMA5D2_BASE_SMC	(SAMA5D2_BASE_HSMC + 0x700)

#define	SAMA5D2_NUM_PIO		4
#define	SAMA5D2_NUM_TWI		2

/* AICREDIR Unlock Key */
#define	SAMA5D2_AICREDIR_KEY		0xB6D81C4D

/*
 * Matrix Slaves ID
 */
/* MATRIX0(H64MX) Matrix Slaves */
/* Bridge from H64MX to AXIMX (Internal ROM, Cryto Library, PKCC RAM) */
#define	SAMA5D2_H64MX_SLAVE_BRIDGE_TO_AXIMX	0
#define	SAMA5D2_H64MX_SLAVE_PERI_BRIDGE		1	/* H64MX Peripheral Bridge */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_0		2	/* DDR2 Port0-AESOTF */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_1		3	/* DDR2 Port1 */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_2		4	/* DDR2 Port2 */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_3		5	/* DDR2 Port3 */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_4		6	/* DDR2 Port4 */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_5		7	/* DDR2 Port5 */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_6		8	/* DDR2 Port6 */
#define	SAMA5D2_H64MX_SLAVE_DDR2_PORT_7		9	/* DDR2 Port7 */
#define	SAMA5D2_H64MX_SLAVE_INTERNAL_SRAM	10	/* Internal SRAM 128K */
#define	SAMA5D2_H64MX_SLAVE_CACHE_L2		11	/* Internal SRAM 128K (Cache L2) */
#define	SAMA5D2_H64MX_SLAVE_QSPI0		12	/* QSPI0 */
#define	SAMA5D2_H64MX_SLAVE_QSPI1		13	/* QSPI1 */
#define	SAMA5D2_H64MX_SLAVE_AESB		14	/* AESB */

/* MATRIX1(H32MX) Matrix Slaves */
#define	SAMA5D2_H32MX_BRIDGE_TO_H64MX		0	/* Bridge from H32MX to H64MX */
#define	SAMA5D2_H32MX_PERI_BRIDGE_0		1	/* H32MX Peripheral Bridge 0 */
#define	SAMA5D2_H32MX_PERI_BRIDGE_1		2	/* H32MX Peripheral Bridge 1 */
#define	SAMA5D2_H32MX_EXTERNAL_EBI		3	/* External Bus Interface */
#define	SAMA5D2_H32MX_NFC_CMD_REG		3	/* NFC command Register */
#define	SAMA5D2_H32MX_NFC_SRAM			4	/* NFC SRAM */
#define	SAMA5D2_H32MX_USB			5

#define	SAMA5D2_SRAM_BASE			SAMA5D2_BASE_SRAM0
#define	SAMA5D2_SRAM_SIZE			(128 * SZ_1K)

static inline void __iomem *sama5d2_pio_map_bank(int bank, unsigned *id)
{
	switch(bank + 'A') {
	case 'A':
		*id = SAMA5D2_ID_PIOA;
		return SAMA5D2_BASE_PIOA;
	case 'B':
		*id = SAMA5D2_ID_PIOB;
		return SAMA5D2_BASE_PIOB;
	case 'C':
		*id = SAMA5D2_ID_PIOC;
		return SAMA5D2_BASE_PIOC;
	case 'D':
		*id = SAMA5D2_ID_PIOD;
		return SAMA5D2_BASE_PIOD;
	}

	return NULL;
}

#define SAMA5D2_BUREG_INDEX	GENMASK(1, 0)
#define SAMA5D2_BUREG_VALID	BIT(2)

#define SAMA5D2_SFC_DR(x)	(SAMA5D2_BASE_SFC + 0x20 + 4 * (x))

#define SAMA5D2_BOOTCFG_QSPI_0		GENMASK(1, 0)
#define SAMA5D2_BOOTCFG_QSPI_1		GENMASK(3, 2)
#define SAMA5D2_BOOTCFG_SPI_0		GENMASK(5, 4)
#define SAMA5D2_BOOTCFG_SPI_1		GENMASK(7, 6)
#define SAMA5D2_BOOTCFG_NFC		GENMASK(9, 8)
#define SAMA5D2_BOOTCFG_SDMMC_0		BIT(10)
#define SAMA5D2_BOOTCFG_SDMMC_1		BIT(11)
#define SAMA5D2_BOOTCFG_UART		GENMASK(15, 12)
#define SAMA5D2_BOOTCFG_JTAG		GENMASK(17, 16)
#define SAMA5D2_BOOTCFG_EXT_MEM_BOOT_EN	BIT(18)
#define SAMA5D2_BOOTCFG_QSPI_XIP	BIT(21)
#define SAMA5D2_DISABLE_BSC_CR		BIT(22)
#define SAMA5D2_DISABLE_MONITOR		BIT(24)
#define SAMA5D2_SECURE_MODE		BIT(29)

static inline u32 sama5d2_bootcfg(void)
{
	u32 bsc_cr, bootcfg = readl(SAMA5D2_SFC_DR(16));

	if (bootcfg & SAMA5D2_DISABLE_BSC_CR)
		return bootcfg;

	bsc_cr = readl(SAMA5D2_BASE_SYSC + 0x54);
	if (bsc_cr & SAMA5D2_BUREG_VALID) {
		u32 __iomem *bureg = SAMA5D2_BASE_SECURAM + 0x1400;
		bootcfg = readl(&bureg[FIELD_GET(SAMA5D2_BUREG_INDEX, bsc_cr)]);
	}

	return bootcfg;
}

#endif
