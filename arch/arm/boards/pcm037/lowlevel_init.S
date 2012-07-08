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

#include <mach/imx-regs.h>
#include <mach/imx-pll.h>

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

	.section ".text_bare_init","ax"

.globl board_init_lowlevel
board_init_lowlevel:

	mov r10, lr
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

	/* Configure IOMUXC
	 * Clears 0x43fa_c26c - 0x43fa_c2dc with 0, except 0x43fa_c278 (untouched), 0x43fa_c27c (set to 0x1000) and 0x43fa_c280 (untouched)
	 * (behaviour copied by sha, source unknown)
	 */
	mov		r1, #0;
	ldr		r0, =0x43FAC26C
	str		r1, [r0], #4
	str		r1, [r0], #4
	str		r1, [r0], #0x10

	ldr		r2, 	=0x43FAC2DC
clear_iomux:
	str		r1, [r0], #4
	cmp		r0, r2
	bls		clear_iomux
	writel(0x1000, 0x43FAC27C )/* CS2 	CSD0) */

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0x80000000
	blo	1f
	cmp	pc, #0x90000000
	bhs	1f

	mov	pc, r10
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

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =0x80f00000		/* Setup a temporary stack in SDRAM */

	ldr	r0, =IMX_NFC_BASE		/* start of NFC SRAM                */
	ldr	r2, =IMX_NFC_BASE + 0x1000	/* end of NFC SRAM                  */

	/* skip NAND boot if not running from NFC space */
	cmp	pc, r0
	blo	ret
	cmp	pc, r2
	bhs	ret

	/* Move ourselves out of NFC SRAM */
	ldr	r1, =_text

copy_loop:
	ldmia	r0!, {r3-r9}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r9}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end address [r2]    */
	ble	copy_loop

	ldr	pc, =1f			/* Jump to SDRAM                    */
1:
	b	nand_boot		/* Load barebox from NAND Flash      */
ret:
#endif /* CONFIG_NAND_IMX_BOOT */

	mov	pc, r10

