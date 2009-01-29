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
#include <asm/arch/esdctl.h>
#include <asm/cache-l2x0.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,	[r0];

#define writeb(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	strb		r1,	[r0];

/* Assuming 24MHz input clock */
#define MPCTL_PARAM_399     (IMX_PLL_PD(0) | IMX_PLL_MFD(15) | IMX_PLL_MFI(8) | IMX_PLL_MFN(5))
#define MPCTL_PARAM_532     (IMX_PLL_PD(1) | IMX_PLL_MFD(0) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))
#define PPCTL_PARAM_300     (IMX_PLL_PD(0) | IMX_PLL_MFD(3) | IMX_PLL_MFI(6) | IMX_PLL_MFN(1))

ARM_PPMRR:              .word   0x40000015
L2CACHE_PARAM:          .word   0x00030024
CCM_CCMR_W:             .word   0x003F4208
CCM_PDR0_W:             .word   0x00801000
MPCTL_PARAM_399_W:      .word   MPCTL_PARAM_399
MPCTL_PARAM_532_W:      .word   MPCTL_PARAM_532
PPCTL_PARAM_W:    	.word   PPCTL_PARAM_300
CCM_BASE_ADDR_W:        .word   IMX_CCM_BASE

.globl board_init_lowlevel
board_init_lowlevel:
	mov     r10, lr

	mrc 15, 0, r1, c1, c0, 0
//	bic r1, r1, #(0x3<<21)
	bic r1, r1, #(0x3<<11)
//	bic r1, r1, #0x5

//	bic r1, r1, #(1<<3)

	mrc 15, 0, r0, c1, c0, 1
	orr r0, r0, #7
	mcr 15, 0, r0, c1, c0, 1

	orr r1, r1, #(1<<11) /* Flow prediction (Z) */
	orr r1, r1, #(1<<22) /* unaligned accesses  */
	orr r1, r1, #(1<<21)

	mcr 15, 0, r1, c1, c0, 0

	mov r0, #0
	mcr 15, 0, r0, c15, c2, 4

	mov r0, #0
	mcr 15, 0, r0, c7, c7, 0        /* invalidate I cache and D cache */
	mcr 15, 0, r0, c8, c7, 0        /* invalidate TLBs */
	mcr 15, 0, r0, c7, c10, 4       /* Drain the write buffer */

	/* Also setup the Peripheral Port Remap register inside the core */
	ldr r0, ARM_PPMRR        /* start from AIPS 2GB region */
	mcr p15, 0, r0, c15, c2, 4

	/* L2 Cache setup / invalidation / disable
	 */
#ifdef L2_INVAL
	/* Disable L2 cache first */
	mov r0, #IMX_L2CC_BASE
	ldr r2, [r0, #L2X0_CTRL]
	bic r2, r2, #0x1
	str r2, [r0, #L2X0_CTRL]
	/*
	 * Configure L2 Cache:
	 * - 128k size(16k way)
	 * - 8-way associativity
	 * - 0 ws TAG/VALID/DIRTY
	 * - 4 ws DATA R/W
	 */
	ldr r1, [r0, #L2X0_AUX_CTRL]
	and r1, r1, #0xFE000000
	ldr r2, L2CACHE_PARAM
	orr r1, r1, r2
	str r1, [r0, #L2X0_AUX_CTRL]

	/* Workaround for DDR issue:WT*/
	ldr r1, [r0, #L2X0_DEBUG_CTRL]
	orr r1, r1, #2
	str r1, [r0, #L2X0_DEBUG_CTRL]

	/* Invalidate L2 */
	mov r1, #0x000000FF
	str r1, [r0, #L2X0_CLEAN_INV_WAY]
L2_loop:
	/* Poll Invalidate By Way register */
	ldr r2, [r0, #L2X0_CLEAN_INV_WAY]
	cmp r2, #0
	bne L2_loop
#endif
/*
 * End of ARM1136 init
 */
	ldr r0, CCM_BASE_ADDR_W

	/* default CLKO to 1/32 of the ARM core*/
	ldr r1, [r0, #CCM_COSR]
	bic r1, r1, #0x00000FF00
	bic r1, r1, #0x0000000FF
	mov r2, #0x00006C00
	add r2, r2, #0x67
	orr r1, r1, r2
	str r1, [r0, #CCM_COSR]

	ldr r2, CCM_CCMR_W
	str r2, [r0, #CCM_CCMR]

	/* check clock path */
	ldr r2, [r0, #CCM_PDR0]
	tst r2, #0x1
	ldrne r3, MPCTL_PARAM_532_W  /* consumer path*/
	ldreq r3, MPCTL_PARAM_399_W  /* auto path*/

	/*Set MPLL , arm clock and ahb clock*/
	str r3, [r0, #CCM_MPCTL]

	ldr r1, PPCTL_PARAM_W
	str r1, [r0, #CCM_PPCTL]

	ldr r1, [r0, #CCM_PDR0]
	orr r1, r1, #0x800000
	str r1, [r0, #CCM_PDR0]

	ldr r1, CCM_PDR0_W
	str r1, [r0, #CCM_PDR0]

	ldr r1, [r0, #CCM_CGR0]
	orr r1, r1, #0x00300000
	str r1, [r0, #CCM_CGR0]

	ldr r1, [r0, #CCM_CGR1]
	orr r1, r1, #0x00000C00
	orr r1, r1, #0x00000003
	str r1, [r0, #CCM_CGR1]

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0x80000000
	bls	1f
	cmp	pc, #0x90000000
	bhi	1f

	mov	pc, lr

1:
	/* MDDR init, enable mDDR*/
	writel(0x00000304, ESDMISC) /* was 0x00000004 */

	/* set timing paramters */
	writel(0x007ffc2f, ESDCFG0) /* was 0x007ffc3f */
	/* select Prechare-All mode */
	writel(0x92220000, ESDCTL0)
	/* Prechare-All */
	writel(0x12345678, IMX_SDRAM_CS0 + 0x400)

	/* select Load-Mode-Register mode */
	writel(0xB8001000, ESDCTL0)
	/* Load reg EMR2 */
	writeb(0xda, 0x84000000)
	/* Load reg EMR3 */
	writeb(0xda, 0x86000000)
	/* Load reg EMR1 -- enable DLL */
	writeb(0xda, 0x82000400)
	/* Load reg MR -- reset DLL */
	writeb(0xda, 0x80000333)

	/* select Prechare-All mode */
	writel(0x92220000, ESDCTL0)
	/* Prechare-All */
	writel(0x12345678, IMX_SDRAM_CS0 + 0x400)

	/* select Manual-Refresh mode */
	writel(0xA2220000, ESDCTL0)
	/* Manual-Refresh 2 times */
	writel(0x87654321, IMX_SDRAM_CS0)
	writel(0x87654321, IMX_SDRAM_CS0)

	/* select Load-Mode-Register mode */
	writel(0xB2220000, ESDCTL0)
	/* Load reg MR -- CL3, BL8, end DLL reset */
	writeb(0xda, 0x80000233)
	/* Load reg EMR1 -- OCD default */
	writeb(0xda, 0x82000780)
	/* Load reg EMR1 -- OCD exit */
	writeb(0xda, 0x82000400)

	/* select normal-operation mode
	 * DSIZ32-bit, BL8, COL10-bit, ROW13-bit
	 * disable PWT & PRCT
	 * disable Auto-Refresh */
	writel(0x82220080, ESDCTL0)

	/* enable Auto-Refresh */
	writel(0x82228080, ESDCTL0)
	/* enable Auto-Refresh */
	writel(0x00002000, ESDCTL1)

	mov r0, #IMX_L2CC_BASE
	ldr r1, [r0, #L2X0_AUX_CTRL]
	orr r1, r1, #0x1000
	str r1, [r0, #L2X0_AUX_CTRL]

	mov pc, lr

