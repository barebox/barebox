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
 */

#ifndef __ASM_ARCH_MX25_REGS_H
#define __ASM_ARCH_MX25_REGS_H

#define MX25_AIPS1_BASE_ADDR		0x43f00000
#define MX25_AIPS1_SIZE			SZ_1M
#define MX25_AIPS2_BASE_ADDR		0x53f00000
#define MX25_AIPS2_SIZE			SZ_1M
#define MX25_AVIC_BASE_ADDR		0x68000000
#define MX25_AVIC_SIZE			SZ_1M

#define MX25_I2C1_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0x80000)
#define MX25_I2C3_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0x84000)
#define MX25_CAN1_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0x88000)
#define MX25_CAN2_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0x8c000)
#define MX25_I2C2_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0x98000)
#define MX25_CSPI1_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0xa4000)
#define MX25_IOMUXC_BASE_ADDR		(MX25_AIPS1_BASE_ADDR + 0xac000)

#define MX25_CCM_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0x80000)
#define MX25_GPT1_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0x90000)
#define MX25_GPIO4_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0x9c000)
#define MX25_PWM2_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xa0000)
#define MX25_GPIO3_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xa4000)
#define MX25_PWM3_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xa8000)
#define MX25_PWM4_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xc8000)
#define MX25_GPIO1_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xcc000)
#define MX25_GPIO2_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xd0000)
#define MX25_WDOG_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xdc000)
#define MX25_PWM1_BASE_ADDR		(MX25_AIPS2_BASE_ADDR + 0xe0000)

#define MX25_UART1_BASE_ADDR		0x43f90000
#define MX25_UART2_BASE_ADDR		0x43f94000
#define MX25_AUDMUX_BASE_ADDR		0x43fb0000
#define MX25_UART3_BASE_ADDR		0x5000c000
#define MX25_UART4_BASE_ADDR		0x50008000
#define MX25_UART5_BASE_ADDR		0x5002c000

#define MX25_CSPI3_BASE_ADDR		0x50004000
#define MX25_CSPI2_BASE_ADDR		0x50010000
#define MX25_FEC_BASE_ADDR		0x50038000
#define MX25_SSI2_BASE_ADDR		0x50014000
#define MX25_SSI1_BASE_ADDR		0x50034000
#define MX25_NFC_BASE_ADDR		0xbb000000
#define MX25_IIM_BASE_ADDR		0x53ff0000
#define MX25_DRYICE_BASE_ADDR		0x53ffc000
#define MX25_ESDHC1_BASE_ADDR		0x53fb4000
#define MX25_ESDHC2_BASE_ADDR		0x53fb8000
#define MX25_LCDC_BASE_ADDR		0x53fbc000
#define MX25_KPP_BASE_ADDR		0x43fa8000
#define MX25_SDMA_BASE_ADDR		0x53fd4000
#define MX25_USB_BASE_ADDR		0x53ff4000
#define MX25_USB_OTG_BASE_ADDR			(MX25_USB_BASE_ADDR + 0x0000)
/*
 * The reference manual (IMX25RM, Rev. 1, 06/2009) specifies an offset of 0x200
 * for the host controller.  Early documentation drafts specified 0x400 and
 * Freescale internal sources confirm only the latter value to work.
 */
#define MX25_USB_HS_BASE_ADDR			(MX25_USB_BASE_ADDR + 0x0400)
#define MX25_CSI_BASE_ADDR		0x53ff8000

#define MX25_IRAM_BASE_ADDR		0x78000000	/* internal ram */
#define MX25_IRAM_SIZE			SZ_128K

/*
 * Clock Controller Module (CCM)
 */
#define MX25_CCM_MPCTL	0x00
#define MX25_CCM_UPCTL	0x04
#define MX25_CCM_CCTL	0x08
#define MX25_CCM_CGCR0	0x0C
#define MX25_CCM_CGCR1	0x10
#define MX25_CCM_CGCR2	0x14
#define MX25_CCM_PCDR0	0x18
#define MX25_CCM_PCDR1	0x1C
#define MX25_CCM_PCDR2	0x20
#define MX25_CCM_PCDR3	0x24
#define MX25_CCM_RCSR	0x28
#define MX25_CCM_CRDR	0x2C
#define MX25_CCM_DCVR0	0x30
#define MX25_CCM_DCVR1	0x34
#define MX25_CCM_DCVR2	0x38
#define MX25_CCM_DCVR3	0x3c
#define MX25_CCM_LTR0	0x40
#define MX25_CCM_LTR1	0x44
#define MX25_CCM_LTR2	0x48
#define MX25_CCM_LTR3	0x4c

#define MX25_PDR0_AUTO_MUX_DIV(x)	(((x) & 0x7) << 9)
#define MX25_PDR0_CCM_PER_AHB(x)	(((x) & 0x7) << 12)
#define MX25_PDR0_CON_MUX_DIV(x)	(((x) & 0xf) << 16)
#define MX25_PDR0_HSP_PODF(x)		(((x) & 0x3) << 20)
#define MX25_PDR0_AUTO_CON		(1 << 0)
#define MX25_PDR0_PER_SEL		(1 << 26)

#define MX25_CCM_RCSR_MEM_CTRL_SHIFT		30
#define MX25_CCM_RCSR_MEM_TYPE_SHIFT		28

/*
 * Adresses and ranges of the external chip select lines
 */
#define MX25_CS0_BASE_ADDR	0xA0000000
#define MX25_CS0_SIZE		SZ_128M
#define MX25_CS1_BASE_ADDR	0xA8000000
#define MX25_CS1_SIZE		SZ_128M
#define MX25_CS2_BASE_ADDR	0xB0000000
#define MX25_CS2_SIZE		SZ_32M
#define MX25_CS3_BASE_ADDR	0xB2000000
#define MX25_CS3_SIZE		SZ_32M
#define MX25_CS4_BASE_ADDR	0xB4000000
#define MX25_CS4_SIZE		SZ_32M
#define MX25_CS5_BASE_ADDR	0xB6000000
#define MX25_CS5_SIZE		SZ_32M

#define MX25_CSD0_BASE_ADDR  0x80000000
#define MX25_CSD1_BASE_ADDR  0x90000000

#define MX25_ESDCTL_BASE_ADDR	0xb8001000
#define MX25_WEIM_BASE_ADDR	0xb8002000

#endif /* __ASM_ARCH_MX25_REGS_H */
