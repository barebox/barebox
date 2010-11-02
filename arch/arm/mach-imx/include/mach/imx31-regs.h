/*
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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

#ifndef __ASM_ARCH_MX31_REGS_H
#define __ASM_ARCH_MX31_REGS_H

/*
 * sanity check
 */
#ifndef _IMX_REGS_H
# error "Please do not include directly. Use imx-regs.h instead."
#endif

#define IMX_OTG_BASE	0x43F88000
#define IMX_UART1_BASE	0x43F90000
#define IMX_UART2_BASE	0x43F94000
#define IMX_WDT_BASE	0x53FDC000
#define IMX_RTC_BASE	0x53FD8000
#define IMX_TIM1_BASE	0x53F90000
#define IMX_IIM_BASE	0x5001C000

#define IMX_SDRAM_CS0	0x80000000
#define IMX_SDRAM_CS1	0x90000000

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

/*
 * Definitions for the clocksource driver
 *
 * These defines are using the i.MX1/27 notation
 * to reuse the clocksource code for these CPUs
 * on the i.MX31
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

#if 0
#define IMX_IO_BASE		0x00200000

/*
 *  Register BASEs, based on OFFSETs
 */
#define IMX_AIPI1_BASE             (0x00000 + IMX_IO_BASE)
#define                (0x01000 + IMX_IO_BASE)
              (0x02000 + IMX_IO_BASE)
#define IMX_TIM2_BASE              (0x03000 + IMX_IO_BASE)
               (0x04000 + IMX_IO_BASE)
#define IMX_LCDC_BASE              (0x05000 + IMX_IO_BASE)
#define IMX_PWM_BASE               (0x08000 + IMX_IO_BASE)
#define IMX_DMAC_BASE              (0x09000 + IMX_IO_BASE)
#define IMX_AIPI2_BASE             (0x10000 + IMX_IO_BASE)
#define IMX_SIM_BASE               (0x11000 + IMX_IO_BASE)
#define IMX_USBD_BASE              (0x12000 + IMX_IO_BASE)
#define IMX_SPI1_BASE              (0x13000 + IMX_IO_BASE)
#define IMX_MMC_BASE               (0x14000 + IMX_IO_BASE)
#define IMX_ASP_BASE               (0x15000 + IMX_IO_BASE)
#define IMX_BTA_BASE               (0x16000 + IMX_IO_BASE)
#define IMX_I2C_BASE               (0x17000 + IMX_IO_BASE)
#define IMX_SSI_BASE               (0x18000 + IMX_IO_BASE)
#define IMX_SPI2_BASE              (0x19000 + IMX_IO_BASE)
#define IMX_MSHC_BASE              (0x1A000 + IMX_IO_BASE)
#define IMX_PLL_BASE               (0x1B000 + IMX_IO_BASE)
#define IMX_SYSCTRL_BASE           (0x1B800 + IMX_IO_BASE)
#define IMX_GPIO_BASE              (0x1C000 + IMX_IO_BASE)
#define IMX_EIM_BASE               (0x20000 + IMX_IO_BASE)
#define IMX_SDRAMC_BASE            (0x21000 + IMX_IO_BASE)
#define IMX_MMA_BASE               (0x22000 + IMX_IO_BASE)
#define IMX_AITC_BASE              (0x23000 + IMX_IO_BASE)
#define IMX_CSI_BASE               (0x24000 + IMX_IO_BASE)
#endif

/*
 * Clock Controller Module (CCM)
 */
#define IMX_CCM_BASE	0x53f80000
#define CCM_CCMR	0x00
#define CCM_PDR0	0x04
#define CCM_PDR1	0x08
#define CCM_RCSR	0x0c
#define CCM_MPCTL	0x10
#define CCM_UPCTL	0x14
#define CCM_SPCTL	0x18
#define CCM_COSR	0x1C

/*
 * ?????????????
 */
#define CCMR_MDS	(1 << 7)
#define CCMR_SBYCS	(1 << 4)
#define CCMR_MPE	(1 << 3)
#define CCMR_PRCS_MASK	(3 << 1)
#define CCMR_FPM	(1 << 1)
#define CCMR_CKIH	(2 << 1)

#define RCSR_NFMS	(1 << 30)

/*
 * ?????????????
 */
#define PDR0_CSI_PODF(x)	(((x) & 0x1ff) << 23)
#define PDR0_PER_PODF(x)	(((x) & 0x1f) << 16)
#define PDR0_HSP_PODF(x)	(((x) & 0x7) << 11)
#define PDR0_NFC_PODF(x)	(((x) & 0x7) << 8)
#define PDR0_IPG_PODF(x)	(((x) & 0x3) << 6)
#define PDR0_MAX_PODF(x)	(((x) & 0x7) << 3)
#define PDR0_MCU_PODF(x)	((x) & 0x7)

#define IMX_ESD_BASE	0xb8001000
#include "esdctl.h"

/*
 * NFC Registers
 */
#define IMX_NFC_BASE               (0xb8000000)

/*
 * Chip Select Registers
 */
#define WEIM_BASE	0xb8002000
#define CSCR_U(x)	(WEIM_BASE + (x) * 0x10)
#define CSCR_L(x)	(WEIM_BASE + 4 + (x) * 0x10)
#define CSCR_A(x)	(WEIM_BASE + 8 + (x) * 0x10)

/*
 * ???????????
 */
#define IOMUXC_BASE	0x43FAC000
#define IOMUXC_GPR	(IOMUXC_BASE + 0x8)
#define IOMUXC_SW_MUX_CTL(x)	(IOMUXC_BASE + 0xc + (x) * 4)
#define IOMUXC_SW_PAD_CTL(x)	(IOMUXC_BASE + 0x154 + (x) * 4)

#define IPU_BASE		0x53fc0000
#define IPU_CONF		IPU_BASE

#define IPU_CONF_PXL_ENDIAN	(1<<8)
#define IPU_CONF_DU_EN		(1<<7)
#define IPU_CONF_DI_EN		(1<<6)
#define IPU_CONF_ADC_EN		(1<<5)
#define IPU_CONF_SDC_EN		(1<<4)
#define IPU_CONF_PF_EN		(1<<3)
#define IPU_CONF_ROT_EN		(1<<2)
#define IPU_CONF_IC_EN		(1<<1)
#define IPU_CONF_SCI_EN		(1<<0)

#define WDOG_BASE		0x53FDC000

/*
 * Signal Multiplexing (IOMUX)
 */

/* bits in the SW_MUX_CTL registers */
#define MUX_CTL_OUT_GPIO_DR	(0 << 4)
#define MUX_CTL_OUT_FUNC	(1 << 4)
#define MUX_CTL_OUT_ALT1	(2 << 4)
#define MUX_CTL_OUT_ALT2	(3 << 4)
#define MUX_CTL_OUT_ALT3	(4 << 4)
#define MUX_CTL_OUT_ALT4	(5 << 4)
#define MUX_CTL_OUT_ALT5	(6 << 4)
#define MUX_CTL_OUT_ALT6	(7 << 4)
#define MUX_CTL_IN_NONE		(0 << 0)
#define MUX_CTL_IN_GPIO		(1 << 0)
#define MUX_CTL_IN_FUNC		(2 << 0)
#define MUX_CTL_IN_ALT1		(4 << 0)
#define MUX_CTL_IN_ALT2		(8 << 0)

#define MUX_CTL_FUNC		(MUX_CTL_OUT_FUNC | MUX_CTL_IN_FUNC)
#define MUX_CTL_ALT1		(MUX_CTL_OUT_ALT1 | MUX_CTL_IN_ALT1)
#define MUX_CTL_ALT2		(MUX_CTL_OUT_ALT2 | MUX_CTL_IN_ALT2)
#define MUX_CTL_GPIO		(MUX_CTL_OUT_GPIO_DR | MUX_CTL_IN_GPIO)

/* Register offsets based on IOMUXC_BASE */
/* 0x00 .. 0x7b */
#define MUX_CTL_RTS1		0x7c
#define MUX_CTL_CTS1		0x7d
#define MUX_CTL_DTR_DCE1	0x7e
#define MUX_CTL_DSR_DCE1	0x7f
#define MUX_CTL_CSPI2_SCLK	0x80
#define MUX_CTL_CSPI2_SPI_RDY	0x81
#define MUX_CTL_RXD1		0x82
#define MUX_CTL_TXD1		0x83
#define MUX_CTL_CSPI2_MISO	0x84
/* 0x85 .. 0x8a */
#define MUX_CTL_CSPI2_MOSI	0x8b

/* The modes a specific pin can be in
 * these macros can be used in mx31_gpio_mux() and have the form
 * MUX_[contact name]__[pin function]
 */
#define MUX_RXD1_UART1_RXD_MUX	((MUX_CTL_FUNC << 8) | MUX_CTL_RXD1)
#define MUX_TXD1_UART1_TXD_MUX	((MUX_CTL_FUNC << 8) | MUX_CTL_TXD1)
#define MUX_RTS1_UART1_RTS_B	((MUX_CTL_FUNC << 8) | MUX_CTL_RTS1)
#define MUX_RTS1_UART1_CTS_B	((MUX_CTL_FUNC << 8) | MUX_CTL_CTS1)

#define MUX_CSPI2_MOSI_I2C2_SCL ((MUX_CTL_ALT1 << 8) | MUX_CTL_CSPI2_MOSI)
#define MUX_CSPI2_MISO_I2C2_SCL ((MUX_CTL_ALT1 << 8) | MUX_CTL_CSPI2_MISO)

#endif /* __ASM_ARCH_MX31_REGS_H */

