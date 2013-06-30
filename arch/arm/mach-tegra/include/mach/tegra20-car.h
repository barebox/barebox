/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Register definitions */
#define CRC_CLK_OUT_ENB_L		0x010
#define CRC_CLK_OUT_ENB_L_CACHE2	(1 << 31)
#define CRC_CLK_OUT_ENB_L_VCP		(1 << 29)
#define CRC_CLK_OUT_ENB_L_HOST1X	(1 << 28)
#define CRC_CLK_OUT_ENB_L_DISP1		(1 << 27)
#define CRC_CLK_OUT_ENB_L_DISP2		(1 << 26)
#define CRC_CLK_OUT_ENB_L_IDE		(1 << 25)
#define CRC_CLK_OUT_ENB_L_3D		(1 << 24)
#define CRC_CLK_OUT_ENB_L_ISP		(1 << 23)
#define CRC_CLK_OUT_ENB_L_USBD		(1 << 22)
#define CRC_CLK_OUT_ENB_L_2D		(1 << 21)
#define CRC_CLK_OUT_ENB_L_VI		(1 << 20)
#define CRC_CLK_OUT_ENB_L_EPP		(1 << 19)
#define CRC_CLK_OUT_ENB_L_I2S2		(1 << 18)
#define CRC_CLK_OUT_ENB_L_PWM		(1 << 17)
#define CRC_CLK_OUT_ENB_L_TWC		(1 << 16)
#define CRC_CLK_OUT_ENB_L_SDMMC4	(1 << 15)
#define CRC_CLK_OUT_ENB_L_SDMMC1	(1 << 14)
#define CRC_CLK_OUT_ENB_L_NDFLASH	(1 << 13)
#define CRC_CLK_OUT_ENB_L_I2C1		(1 << 12)
#define CRC_CLK_OUT_ENB_L_I2S1		(1 << 11)
#define CRC_CLK_OUT_ENB_L_SPDIF		(1 << 10)
#define CRC_CLK_OUT_ENB_L_SDMMC2	(1 << 9)
#define CRC_CLK_OUT_ENB_L_GPIO		(1 << 8)
#define CRC_CLK_OUT_ENB_L_UART2		(1 << 7)
#define CRC_CLK_OUT_ENB_L_UART1		(1 << 6)
#define CRC_CLK_OUT_ENB_L_TMR		(1 << 5)
#define CRC_CLK_OUT_ENB_L_RTC		(1 << 4)
#define CRC_CLK_OUT_ENB_L_AC97		(1 << 3)
#define CRC_CLK_OUT_ENB_L_CPU		(1 << 0)

#define CRC_SCLK_BURST_POLICY		0x028
#define CRC_SCLK_BURST_POLICY_SYS_STATE_SHIFT	28
#define CRC_SCLK_BURST_POLICY_SYS_STATE_FIQ	8
#define CRC_SCLK_BURST_POLICY_SYS_STATE_IRQ	4
#define CRC_SCLK_BURST_POLICY_SYS_STATE_RUN	2
#define CRC_SCLK_BURST_POLICY_SYS_STATE_IDLE	1
#define CRC_SCLK_BURST_POLICY_SYS_STATE_STDBY	0

#define CRC_SUPER_SCLK_DIV		0x02c
#define CRC_SUPER_SDIV_ENB		(1 << 31)
#define CRC_SUPER_SDIV_DIS_FROM_COP_FIQ	(1 << 27)
#define CRC_SUPER_SDIV_DIS_FROM_CPU_FIQ	(1 << 26)
#define CRC_SUPER_SDIV_DIS_FROM_COP_IRQ	(1 << 25)
#define CRC_SUPER_SDIV_DIS_FROM_CPU_IRQ	(1 << 24)
#define CRC_SUPER_SDIV_DIVIDEND_SHIFT	8
#define CRC_SUPER_SDIV_DIVIDEND_MASK	(0xff << CRC_SUPER_SDIV_DIVIDEND_SHIFT)
#define CRC_SUPER_SDIV_DIVISOR_SHIFT	0
#define CRC_SUPER_SDIV_DIVISOR_MASK	(0xff << CRC_SUPER_SDIV_DIVISOR_SHIFT)

#define CRC_CLK_CPU_CMPLX		0x04c
#define CRC_CLK_CPU_CMPLX_CPU3_CLK_STP	(1 << 11)
#define CRC_CLK_CPU_CMPLX_CPU2_CLK_STP	(1 << 10)
#define CRC_CLK_CPU_CMPLX_CPU1_CLK_STP	(1 << 9)
#define CRC_CLK_CPU_CMPLX_CPU0_CLK_STP	(1 << 8)
#define CRC_CLK_CPU_CMPLX_CPU_BRIDGE_DIV_SHIFT	0
#define CRC_CLK_CPU_CMPLX_CPU_BRIDGE_DIV_4	3
#define CRC_CLK_CPU_CMPLX_CPU_BRIDGE_DIV_3	2
#define CRC_CLK_CPU_CMPLX_CPU_BRIDGE_DIV_2	1
#define CRC_CLK_CPU_CMPLX_CPU_BRIDGE_DIV_1	0

#define CRC_OSC_CTRL			0x050
#define CRC_OSC_CTRL_OSC_FREQ_SHIFT	30
#define CRC_OSC_CTRL_OSC_FREQ_MASK	(0x3 << CRC_OSC_CTRL_OSC_FREQ_SHIFT)
#define CRC_OSC_CTRL_PLL_REF_DIV_SHIFT	28
#define CRC_OSC_CTRL_PLL_REF_DIV_MASK	(0x3 << CRC_OSC_CTRL_PLL_REF_DIV_SHIFT)

#define CRC_PLL_BASE_LOCK		27
#define CRC_PLLE_MISC_LOCK		11

#define CRC_PLL_MISC_LOCK_ENABLE	18
#define CRC_PLLDU_MISC_LOCK_ENABLE	22
#define CRC_PLLE_MISC_LOCK_ENABLE	9

#define CRC_PLLS_BASE			0x0f0
#define CRC_PLLS_MISC			0x0f4

#define CRC_PLLC_BASE			0x080
#define CRC_PLLC_OUT			0x084
#define CRC_PLLC_MISC			0x08c

#define CRC_PLLM_BASE			0x090
#define CRC_PLLM_OUT			0x094
#define CRC_PLLM_MISC			0x09c

#define CRC_PLLP_BASE			0x0a0
#define CRC_PLLP_OUTA			0x0a4
#define CRC_PLLP_OUTB			0x0a8
#define CRC_PLLP_MISC			0x0ac

#define CRC_PLLA_BASE			0x0b0
#define CRC_PLLA_OUT			0x0b4
#define CRC_PLLA_MISC			0x0bc

#define CRC_PLLU_BASE			0x0c0
#define CRC_PLLU_MISC			0x0cc

#define CRC_PLLD_BASE			0x0d0
#define CRC_PLLD_MISC			0x0dc

#define CRC_PLLX_BASE			0x0e0
#define CRC_PLLX_BASE_BYPASS		(1 << 31)
#define CRC_PLLX_BASE_ENABLE		(1 << 30)
#define CRC_PLLX_BASE_REF_DIS		(1 << 29)
#define CRC_PLLX_BASE_LOCK		(1 << 27)
#define CRC_PLLX_BASE_DIVP_SHIFT	20
#define CRC_PLLX_BASE_DIVP_MASK		(0x7 << CRC_PLLX_BASE_DIVP_SHIFT)
#define CRC_PLLX_BASE_DIVN_SHIFT	8
#define CRC_PLLX_BASE_DIVN_MASK		(0x3ff << CRC_PLLX_BASE_DIVN_SHIFT)
#define CRC_PLLX_BASE_DIVM_SHIFT	0
#define CRC_PLLX_BASE_DIVM_MASK		(0xf << CRC_PLLX_BASE_DIVM_SHIFT)

#define CRC_PLLX_MISC			0x0e4
#define CRC_PLLX_MISC_SETUP_SHIFT	24
#define CRC_PLLX_MISC_SETUP_MASK	(0xf << CRC_PLLX_MISC_SETUP_SHIFT)
#define CRC_PLLX_MISC_PTS_SHIFT		22
#define CRC_PLLX_MISC_PTS_MASK		(0x3 << CRC_PLLX_MISC_PTS_SHIFT)
#define CRC_PLLX_MISC_DCCON		(1 << 20)
#define CRC_PLLX_MISC_LOCK_ENABLE	(1 << 18)
#define CRC_PLLX_MISC_LOCK_SEL_SHIFT	12
#define CRC_PLLX_MISC_LOCK_SEL_MASK	(0x3f << CRC_PLLX_MISC_LOCK_SEL_SHIFT)
#define CRC_PLLX_MISC_CPCON_SHIFT	8
#define CRC_PLLX_MISC_CPCON_MASK	(0xf << CRC_PLLX_MISC_CPCON_SHIFT)
#define CRC_PLLX_MISC_LFCON_SHIFT	4
#define CRC_PLLX_MISC_LFCON_MASK	(0xf << CRC_PLLX_MISC_LFCON_SHIFT)
#define CRC_PLLX_MISC_VCOCON_SHIFT	0
#define CRC_PLLX_MISC_VCOCON_MASK	(0xf << CRC_PLLX_MISC_VCOCON_SHIFT)

#define CRC_PLLE_BASE			0x0e8
#define CRC_PLLE_MISC			0x0ec

#define CRC_RST_DEV_L_SET		0x300
#define CRC_RST_DEV_L_CACHE2		(1 << 31)
#define CRC_RST_DEV_L_VCP		(1 << 29)
#define CRC_RST_DEV_L_HOST1X		(1 << 28)
#define CRC_RST_DEV_L_DISP1		(1 << 27)
#define CRC_RST_DEV_L_DISP2		(1 << 26)
#define CRC_RST_DEV_L_IDE		(1 << 25)
#define CRC_RST_DEV_L_3D		(1 << 24)
#define CRC_RST_DEV_L_ISP		(1 << 23)
#define CRC_RST_DEV_L_USBD		(1 << 22)
#define CRC_RST_DEV_L_2D		(1 << 21)
#define CRC_RST_DEV_L_VI		(1 << 20)
#define CRC_RST_DEV_L_EPP		(1 << 19)
#define CRC_RST_DEV_L_I2S2		(1 << 18)
#define CRC_RST_DEV_L_PWM		(1 << 17)
#define CRC_RST_DEV_L_TWC		(1 << 16)
#define CRC_RST_DEV_L_SDMMC4		(1 << 15)
#define CRC_RST_DEV_L_SDMMC1		(1 << 14)
#define CRC_RST_DEV_L_NDFLASH		(1 << 13)
#define CRC_RST_DEV_L_I2C1		(1 << 12)
#define CRC_RST_DEV_L_I2S1		(1 << 11)
#define CRC_RST_DEV_L_SPDIF		(1 << 10)
#define CRC_RST_DEV_L_SDMMC2		(1 << 9)
#define CRC_RST_DEV_L_GPIO		(1 << 8)
#define CRC_RST_DEV_L_UART2		(1 << 7)
#define CRC_RST_DEV_L_UART1		(1 << 6)
#define CRC_RST_DEV_L_TMR		(1 << 5)
#define CRC_RST_DEV_L_AC97		(1 << 3)
#define CRC_RST_DEV_L_SYS		(1 << 2)
#define CRC_RST_DEV_L_COP		(1 << 1)
#define CRC_RST_DEV_L_CPU		(1 << 0)

#define CRC_RST_DEV_L_CLR		0x304

#define CRC_RST_CPU_CMPLX_SET		0x340

#define CRC_RST_CPU_CMPLX_CLR		0x344
