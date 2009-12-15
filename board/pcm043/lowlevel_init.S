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
#include <mach/esdctl.h>
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
#define MPCTL_PARAM_532     ((1 << 31) | IMX_PLL_PD(0) | IMX_PLL_MFD(11) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))
#define PPCTL_PARAM_300     (IMX_PLL_PD(0) | IMX_PLL_MFD(3) | IMX_PLL_MFI(6) | IMX_PLL_MFN(1))

	.section ".text_bare_init","ax"

ARM_PPMRR:              .word   0x40000015
L2CACHE_PARAM:          .word   0x00030024
CCM_CCMR_W:             .word   0x003F4208
CCM_PDR0_W:             .word   0x00001000
MPCTL_PARAM_399_W:      .word   MPCTL_PARAM_399
MPCTL_PARAM_532_W:      .word   MPCTL_PARAM_532
PPCTL_PARAM_W:    	.word   PPCTL_PARAM_300
CCM_BASE_ADDR_W:        .word   IMX_CCM_BASE

.globl board_init_lowlevel
board_init_lowlevel:
	mov     r10, lr

	mrc 15, 0, r1, c1, c0, 0

	mrc 15, 0, r0, c1, c0, 1
	orr r0, r0, #7
	mcr 15, 0, r0, c1, c0, 1

	orr r1, r1, #(1<<11) /* Flow prediction (Z) */
	orr r1, r1, #(1<<22) /* unaligned accesses  */
	orr r1, r1, #(1<<21) /* Low Int Latency     */

	mcr 15, 0, r1, c1, c0, 0

	mov r0, #0
	mcr 15, 0, r0, c15, c2, 4

	/*
	 * Branch predicition is now enabled.  Flush the BTAC to ensure a valid 
	 * starting point.  Don't flush BTAC while it is disabled to avoid
	 * ARM1136 erratum 408023.
	 */
	mov     r0, #0
	mcr     p15, 0, r0, c7, c5, 6	/* flush entire BTAC */

	mov r0, #0
	mcr 15, 0, r0, c7, c7, 0        /* invalidate I cache and D cache */
	mcr 15, 0, r0, c8, c7, 0        /* invalidate TLBs */
	mcr 15, 0, r0, c7, c10, 4       /* Drain the write buffer */

	/* Also setup the Peripheral Port Remap register inside the core */
	ldr r0, ARM_PPMRR        /* start from AIPS 2GB region */
	mcr p15, 0, r0, c15, c2, 4

/*
 * End of ARM1136 init
 */
	ldr r0, CCM_BASE_ADDR_W

	ldr r2, CCM_CCMR_W
	str r2, [r0, #CCM_CCMR]

	ldr r3, MPCTL_PARAM_399_W  /* consumer path*/

	/*Set MPLL , arm clock and ahb clock*/
	str r3, [r0, #CCM_MPCTL]

	ldr r1, PPCTL_PARAM_W
	str r1, [r0, #CCM_PPCTL]

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

	mov	pc, r10

1:
	/* Set DDR Type to SDRAM, drive strength workaround	*
	 * 0x00000000	MDDR					*
	 * 0x00000800	3,3V SDRAM				*/

	#define DDRTYPE	0x00000800

	ldr	r0, =IMX_IOMUXC_BASE + 0x794
	ldr	r1, =DDRTYPE
	str	r1, [r0], #4	/* IMX_IOMUXC_BASE + 0x794 */
	str	r1, [r0], #4	/* IMX_IOMUXC_BASE + 0x798 */
	str	r1, [r0], #4	/* IMX_IOMUXC_BASE + 0x79c */
	str	r1, [r0], #4	/* IMX_IOMUXC_BASE + 0x7a0 */
	str	r1, [r0]	/* IMX_IOMUXC_BASE + 0x7a4 */

	/* MDDR init, enable mDDR*/
	writel(0x00000304, ESDMISC) /* was 0x00000004 */

	/* set timing paramters */
	writel(0x00255417, ESDCFG0)
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

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =TEXT_BASE - 4	/* Setup a temporary stack in SDRAM */

	ldr	r0, =IMX_NFC_BASE		/* start of NFC SRAM                */
	ldr	r2, =IMX_NFC_BASE + 0x800	/* end of NFC SRAM                  */

	/* skip NAND boot if not running from NFC space */
	cmp	pc, r0
	blo	ret
	cmp	pc, r2
	bhs	ret

	/* Move ourselves out of NFC SRAM */
	ldr	r1, =TEXT_BASE

copy_loop:
	ldmia	r0!, {r3-r9}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r9}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end addreee [r2]    */
	ble	copy_loop

	ldr	pc, =1f			/* Jump to SDRAM                    */
1:
	bl	nand_boot		/* Load barebox from NAND Flash      */

	/* rebase the return address */
	ldr	r1, =IMX_NFC_BASE - TEXT_BASE
	sub	r10, r10, r1		/* adjust return address from NFC SRAM */
ret:
#endif /* CONFIG_NAND_IMX_BOOT */

	mov pc, r10

