/*
 *
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

#include <asm/arch/imx-regs.h>
#include <asm/arch/imx-pll.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#define writeb(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	strb		r1,   [r0];

.macro DELAY loops
	ldr r2, =\loops
1:
	subs	r2, r2, #1
	nop
	bcs 1b
.endm

.globl board_init_lowlevel
board_init_lowlevel:

	writel(IPU_CONF_DI_EN, IPU_CONF)
	writel(0x074B0BF5, IMX_CCM_BASE + CCM_CCMR)

	DELAY 0x40000

	writel(0x074B0BF5 | CCMR_MPE, IMX_CCM_BASE + CCM_CCMR)
	writel((0x074B0BF5 | CCMR_MPE) & ~CCMR_MDS, IMX_CCM_BASE + CCM_CCMR)

	writel(PDR0_CSI_PODF(0xff1) | \
		PDR0_PER_PODF(7) | \
		PDR0_HSP_PODF(3) | \
		PDR0_NFC_PODF(5) | \
		PDR0_IPG_PODF(1) | \
		PDR0_MAX_PODF(3) | \
		PDR0_MCU_PODF(0), \
		IMX_CCM_BASE + CCM_PDR0)

	writel(IMX_PLL_PD(0) | IMX_PLL_MFD(0xe) | IMX_PLL_MFI(9) | IMX_PLL_MFN(0xd), IMX_CCM_BASE + CCM_MPCTL)
	writel(IMX_PLL_PD(1) | IMX_PLL_MFD(0x43) | IMX_PLL_MFI(12) | IMX_PLL_MFN(1), IMX_CCM_BASE + CCM_SPCTL)

	/* Configure IOMUXC */
	writel(0, 0x43FAC26C)/* SDCLK */
	writel(0, 0x43FAC270) /* CAS */
	writel(0, 0x43FAC274) /* RAS */
	writel(0x1000, 0x43FAC27C )/* CS2 	CSD0) */
	writel(0, 0x43FAC284) /* DQM3 */
	writel(0, 0x43FAC288) /* DQM2, DQM1, DQM0, SD31-SD0, A25-A0, MA10 	0x288..0x2DC) */
	writel(0, 0x43FAC28C)
	writel(0, 0x43FAC290)
	writel(0, 0x43FAC294)
	writel(0, 0x43FAC298)
	writel(0, 0x43FAC29C)
	writel(0, 0x43FAC2A0)
	writel(0, 0x43FAC2A4)
	writel(0, 0x43FAC2A8)
	writel(0, 0x43FAC2AC)
	writel(0, 0x43FAC2B0)
	writel(0, 0x43FAC2B4)
	writel(0, 0x43FAC2B8)
	writel(0, 0x43FAC2BC)
	writel(0, 0x43FAC2C0)
	writel(0, 0x43FAC2C4)
	writel(0, 0x43FAC2C8)
	writel(0, 0x43FAC2CC)
	writel(0, 0x43FAC2D0)
	writel(0, 0x43FAC2D4)
	writel(0, 0x43FAC2D8)
	writel(0, 0x43FAC2DC)

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0x80000000
	bls	1f
	cmp	pc, #0x90000000
	bhi	1f

	mov	pc, lr

1:

#if defined CONFIG_PCM037_SDRAM_BANK0_128MB
#define ROWS0	ESDCTL0_ROW13
#elif defined CONFIG_PCM037_SDRAM_BANK0_256MB
#define ROWS0	ESDCTL0_ROW14
#endif
	writel(0x00000004, ESDMISC)
	writel(0x006ac73a, ESDCFG0)
	writel(0x90100000 | ROWS0, ESDCTL0)
	writel(0x12344321, IMX_SDRAM_CS0 + 0xf00)
	writel(0xa0100000 | ROWS0, ESDCTL0)
	writel(0x12344321, IMX_SDRAM_CS0)
	writel(0x12344321, IMX_SDRAM_CS0)
	writel(0xb0100000 | ROWS0, ESDCTL0)
	writeb(0xda, IMX_SDRAM_CS0 + 0x33) 
	writeb(0xff, IMX_SDRAM_CS0 + 0x01000000)
	writel(0x80226080 | ROWS0, ESDCTL0)
	writel(0xDEADBEEF, IMX_SDRAM_CS0)
	writel(0x0000000c, ESDMISC)

#ifndef CONFIG_PCM037_SDRAM_BANK1_NONE
#if defined CONFIG_PCM037_SDRAM_BANK1_128MB
#define ROWS1	ESDCTL0_ROW13
#elif defined CONFIG_PCM037_SDRAM_BANK1_256MB
#define ROWS1	ESDCTL0_ROW14
#endif
	writel(0x006ac73a, ESDCFG1)
	writel(0x90100000 | ROWS1, ESDCTL1)
	writel(0x12344321, IMX_SDRAM_CS1 + 0xf00)
	writel(0xa0100000 | ROWS1, ESDCTL1)
	writel(0x12344321, IMX_SDRAM_CS1)
	writel(0x12344321, IMX_SDRAM_CS1)
	writel(0xb0100000 | ROWS1, ESDCTL1)
	writeb(0xda, IMX_SDRAM_CS1 + 0x33) 
	writeb(0xff, IMX_SDRAM_CS1 + 0x01000000)
	writel(0x80226080 | ROWS1, ESDCTL1)
	writel(0xDEADBEEF, IMX_SDRAM_CS1)
	writel(0x0000000c, ESDMISC)
#endif
	mov	pc, lr

