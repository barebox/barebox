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
#define IMX_NFC_BASE		0xBB000000
#define IMX_FEC_BASE		0x50038000
#define IMX_I2C1_BASE		0x43F80000
#define IMX_I2C2_BASE		0x43F98000
#define IMX_I2C3_BASE		0x43F84000
#define IMX_SDHC1_BASE		0x53FB4000
#define IMX_SDHC2_BASE		0x53FB8000
#define IMX_SDHC3_BASE		0x53FBC000
#define IMX_IPU_BASE		0x53FC0000
#define IMX_OTG_BASE		0x53FF4000
#define IMX_WDOG_BASE		0x53fdc000

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

#define CCM_CGR1_FEC_SHIFT	0
#define CCM_CGR1_I2C1_SHIFT	10
#define CCM_CGR1_SDHC1_SHIFT	26
#define CCM_CGR2_USB_SHIFT	22

#define CCM_RCSR_MEM_CTRL_SHIFT		25
#define CCM_RCSR_MEM_TYPE_SHIFT		23

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

#endif /* __ASM_ARCH_MX35_REGS_H */

