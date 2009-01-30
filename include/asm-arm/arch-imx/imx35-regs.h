/*
 * (c) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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

#ifndef __ASM_ARCH_MX35_REGS_H
#define __ASM_ARCH_MX35_REGS_H

/*
 * sanity check
 */
#ifndef _IMX_REGS_H
# error "Please do not include directly. Use imx-regs.h instead."
#endif

#define IMX_L2CC_BASE		0x30000000
#define IMX_UART1_BASE		0x43F90000
#define IMX_UART2_BASE		0x43F94000
#define IMX_TIM1_BASE		0x53F90000
#define IMX_IOMUXC_BASE		0x43FAC000
#define IMX_WDT_BASE		0x53FDC000
#define IMX_MAX_BASE		0x43F04000
#define IMX_ESD_BASE		0xb8001000
#define IMX_AIPS1_BASE		0x43F00000
#define IMX_AIPS2_BASE		0x53F00000
#define IMX_CCM_BASE		0x53F80000
#define IMX_IIM_BASE		0x53FF0000
#define IMX_M3IF_BASE		0xB8003000
#define IMX_NAND_BASE		0xBB000000

/*
 * Clock Controller Module (CCM)
 */
#define CCM_CCMR	0x00
#define CCM_PDR0	0x04
#define CCM_PDR1	0x08
#define CCM_PDR2	0x0C
#define CCM_PDR3	0x10
#define CCM_PDR4	0x14
#define CCM_RCSR	0x18
#define CCM_MPCTL	0x1C
#define CCM_PPCTL	0x20
#define CCM_ACMR	0x24
#define CCM_COSR	0x28
#define CCM_CGR0	0x2C
#define CCM_CGR1	0x30
#define CCM_CGR2	0x34
#define CCM_CGR3	0x38

#define PDR0_AUTO_MUX_DIV(x)	(((x) & 0x7) << 9)
#define PDR0_CCM_PER_AHB(x)	(((x) & 0x7) << 12)
#define PDR0_CON_MUX_DIV(x)	(((x) & 0xf) << 16)
#define PDR0_HSP_PODF(x)	(((x) & 0x3) << 20)
#define PDR0_AUTO_CON		(1 << 0)
#define PDR0_PER_SEL		(1 << 26)

/*
 * Adresses and ranges of the external chip select lines
 */
#define IMX_CS0_BASE	0xA0000000
#define IMX_CS0_RANGE	(128 * 1024 * 1024)
#define IMX_CS1_BASE	0xA8000000
#define IMX_CS1_RANGE	(128 * 1024 * 1024)
#define IMX_CS2_BASE	0xB0000000
#define IMX_CS2_RANGE	(32 * 1024 * 1024)
#define IMX_CS3_BASE	0xB2000000
#define IMX_CS3_RANGE	(32 * 1024 * 1024)
#define IMX_CS4_BASE	0xB4000000
#define IMX_CS4_RANGE	(32 * 1024 * 1024)
#define IMX_CS5_BASE	0xB6000000
#define IMX_CS5_RANGE	(32 * 1024 * 1024)

#define IMX_SDRAM_CS0  0x80000000
#define IMX_SDRAM_CS1  0x90000000

#define WEIM_BASE 0xb8002000
#define CSCR_U(x)     (WEIM_BASE + (x) * 0x10)
#define CSCR_L(x)     (WEIM_BASE + 4 + (x) * 0x10)
#define CSCR_A(x)     (WEIM_BASE + 8 + (x) * 0x10)

/*
 * Definitions for the clocksource driver
 *
 * These defines are using the i.MX1/27 notation
 * to reuse the clocksource code for these CPUs
 * on the i.MX35
 */
/* Part 1: Registers */
#define GPT_TCTL   0x00
#define GPT_TPRER  0x04
#define GPT_TCMP   0x10
#define GPT_TCR    0x1c
#define GPT_TCN    0x24
#define GPT_TSTAT  0x08

/* Part 2: Bitfields */
#define TCTL_SWR       (1<<15) /* Software reset */
#define TCTL_FRR       (1<<9)  /* Freerun / restart */
#define TCTL_CAP       (3<<6)  /* Capture Edge */
#define TCTL_OM        (1<<5)  /* output mode */
#define TCTL_IRQEN     (1<<4)  /* interrupt enable */
#define TCTL_CLKSOURCE (6)     /* Clock source bit position */
#define TCTL_TEN       (1)     /* Timer enable */
#define TPRER_PRES     (0xff)  /* Prescale */
#define TSTAT_CAPT     (1<<1)  /* Capture event */
#define TSTAT_COMP     (1)     /* Compare event */

/*
 * Watchdog Registers
 */
#define WCR  __REG16(IMX_WDT_BASE + 0x00) /* Watchdog Control Register */
#define WSR  __REG16(IMX_WDT_BASE + 0x02) /* Watchdog Service Register */
#define WSTR __REG16(IMX_WDT_BASE + 0x04) /* Watchdog Status Register  */

/* important definition of some bits of WCR */
#define WCR_WDE 0x04

/* bits in the SW_MUX_CTL registers */
#define MUX_CTL_ALT0	(0 << 0)
#define MUX_CTL_ALT1	(1 << 0)
#define MUX_CTL_ALT2	(2 << 0)
#define MUX_CTL_ALT3	(3 << 0)
#define MUX_CTL_ALT4	(4 << 0)
#define MUX_CTL_ALT5	(5 << 0)
#define MUX_CTL_ALT6	(6 << 0)
#define MUX_CTL_ALT7	(7 << 0)

#define MUX_CTL_PAD_SION	(1 << 4)

/* Register offsets based on IOMUXC_BASE */
#define MUX_CTL_COMPARE		0x8
#define MUX_CTL_WDOG_RST	0xc

#define MUX_CTL_I2C1_CLK	0x110
#define MUX_CTL_I2C1_DAT	0x114
#define MUX_CTL_I2C2_CLK	0x118

#define MUX_CTL_STXD5      	0x130
#define MUX_CTL_SRXD5		0x134
#define MUX_CTL_SCK5		0x138
#define MUX_CTL_STXFS5		0x13c

#define MUX_CTL_TX5_RX0		0x158
#define MUX_CTL_TX4_RX1		0x15c

#define MUX_CTL_CSPI1_MOSI      0x170
#define MUX_CTL_CSPI1_MISO	0x174
#define MUX_CTL_CSPI1_SS0	0x178
#define MUX_CTL_CSPI1_SS1	0x17c
#define MUX_CTL_CSPI1_SCLK      0x180
#define MUX_CTL_CSPI1_SPI_RDY   0x184
#define MUX_CTL_RXD1		0x188
#define MUX_CTL_TXD1		0x18c
#define MUX_CTL_RTS1		0x190
#define MUX_CTL_CTS1		0x194

#define MUX_CTL_ATA_RESET_B	0x274

#define MUX_CTL_FEC_TX_CLK	0x2e0
#define MUX_CTL_FEC_RX_CLK	0x2e4
#define MUX_CTL_FEC_RX_DV	0x2e8
#define MUX_CTL_FEC_COL		0x2ec
#define MUX_CTL_FEC_RDATA0	0x2f0
#define MUX_CTL_FEC_TDATA0	0x2f4
#define MUX_CTL_FEC_TX_EN	0x2f8
#define MUX_CTL_FEC_MDC		0x2fc
#define MUX_CTL_FEC_MDIO	0x300
#define MUX_CTL_FEC_TX_ERR	0x304
#define MUX_CTL_FEC_RX_ERR	0x308
#define MUX_CTL_FEC_CRS		0x30c
#define MUX_CTL_FEC_RDATA1	0x310
#define MUX_CTL_FEC_TDATA1	0x314
#define MUX_CTL_FEC_RDATA2	0x318
#define MUX_CTL_FEC_TDATA2	0x31c
#define MUX_CTL_FEC_RDATA3	0x320
#define MUX_CTL_FEC_TDATA3	0x324

#define PAD_CTL_COMPARE		0x32c
#define PAD_CTL_WDOG_RST	0x330

#define PAD_CTL_I2C1_CLK	0x554
#define PAD_CTL_I2C1_DAT	0x558
#define PAD_CTL_I2C2_CLK	0x55c

#define PAD_CTL_STXD5		0x574
#define PAD_CTL_SRXD5		0x578
#define PAD_CTL_SCK5		0x57c
#define PAD_CTL_STXFS5		0x580

#define PAD_CTL_TX5_RX0		0x59c
#define PAD_CTL_TX4_RX1		0x5a0

#define PAD_CTL_CSPI1_MOSI      0x5b4
#define PAD_CTL_CSPI1_MISO	0x5b8
#define PAD_CTL_CSPI1_SS0	0x5bc
#define PAD_CTL_CSPI1_SS1	0x5c0
#define PAD_CTL_CSPI1_SCLK      0x5c4
#define PAD_CTL_CSPI1_SPI_RDY   0x5c8
#define PAD_CTL_RXD1		0x5cc
#define PAD_CTL_TXD1		0x5d0
#define PAD_CTL_RTS1		0x5d4
#define PAD_CTL_CTS1		0x5d8

#define PAD_CTL_ATA_RESET_B	0x6d8

#define PAD_CTL_FEC_TX_CLK	0x744
#define PAD_CTL_FEC_RX_CLK	0x748
#define PAD_CTL_FEC_RX_DV	0x74c
#define PAD_CTL_FEC_COL		0x750
#define PAD_CTL_FEC_RDATA0	0x754
#define PAD_CTL_FEC_TDATA0	0x758
#define PAD_CTL_FEC_TX_EN	0x75c
#define PAD_CTL_FEC_MDC		0x760
#define PAD_CTL_FEC_MDIO	0x764
#define PAD_CTL_FEC_TX_ERR	0x768
#define PAD_CTL_FEC_RX_ERR	0x76c
#define PAD_CTL_FEC_CRS		0x770
#define PAD_CTL_FEC_RDATA1	0x774
#define PAD_CTL_FEC_TDATA1	0x778
#define PAD_CTL_FEC_RDATA2	0x77c
#define PAD_CTL_FEC_TDATA2	0x780
#define PAD_CTL_FEC_RDATA3	0x784
#define PAD_CTL_FEC_TDATA3	0x788

/* The modes a specific pin can be in
 * these macros can be used in mx31_gpio_mux() and have the form
 * MUX_[contact name]__[pin function]
 */

#define MUX_RXD1_UART1_RXD_MUX		((MUX_CTL_ALT0 << 16) | MUX_CTL_RXD1)
#define MUX_TXD1_UART1_TXD_MUX		((MUX_CTL_ALT0 << 16) | MUX_CTL_TXD1)
#define MUX_RTS1_UART1_RTS_B		((MUX_CTL_ALT0 << 16) | MUX_CTL_RTS1)
#define MUX_RTS1_UART1_CTS_B		((MUX_CTL_ALT0 << 16) | MUX_CTL_CTS1)

#define MUX_I2C1_CLK_I2C1_SLC		((MUX_CTL_ALT0 << 16) | MUX_CTL_I2C1_CLK)
#define MUX_I2C1_DAT_I2C1_SDA		((MUX_CTL_ALT0 << 16) | MUX_CTL_I2C1_DAT)

#define MUX_FEC_TX_CLK_FEC_TX_CLK 	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TX_CLK)
#define MUX_FEC_RX_CLK_FEC_RX_CLK	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RX_CLK)
#define MUX_FEC_RX_DV_FEC_RX_DV		((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RX_DV)
#define MUX_FEC_COL_FEC_COL		((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_COL)
#define MUX_FEC_TX_EN_FEC_TX_EN		((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TX_EN)
#define MUX_FEC_MDC_FEC_MDC		((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_MDC)
#define MUX_FEC_MDIO_FEC_MDIO		((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_MDIO)
#define MUX_FEC_TX_ERR_FEC_TX_ERR	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TX_ERR)
#define MUX_FEC_RX_ERR_FEC_RX_ERR	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RX_ERR)
#define MUX_FEC_CRS_FEC_CRS		((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_CRS)
#define MUX_FEC_RDATA0_FEC_RDATA0	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RDATA0)
#define MUX_FEC_TDATA0_FEC_TDATA0	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TDATA0)
#define MUX_FEC_RDATA1_FEC_RDATA1	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RDATA1)
#define MUX_FEC_TDATA1_FEC_TDATA1	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TDATA1)
#define MUX_FEC_RDATA2_FEC_RDATA2	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RDATA2)
#define MUX_FEC_TDATA2_FEC_TDATA2	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TDATA2)
#define MUX_FEC_RDATA3_FEC_RDATA3	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_RDATA3)
#define MUX_FEC_TDATA3_FEC_TDATA3	((MUX_CTL_ALT0 << 16) | PAD_CTL_FEC_TDATA3)

#endif /* __ASM_ARCH_MX35_REGS_H */

