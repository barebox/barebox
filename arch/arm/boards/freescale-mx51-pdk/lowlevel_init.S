/*
 * This code is based on the ecos babbage startup code
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

#include <config.h>
#include <mach/imx-regs.h>
#include <mach/clock-imx51.h>

#define ROM_SI_REV_OFFSET                   0x48

.macro setup_pll pll, freq
	ldr r2, =\pll
	ldr r1, =0x00001232
	str r1, [r2, #MX51_PLL_DP_CTL] /* Set DPLL ON (set UPEN bit): BRMO=1 */
	mov r1, #0x2
	str r1, [r2, #MX51_PLL_DP_CONFIG] /* Enable auto-restart AREN bit */

	str r3, [r2, #MX51_PLL_DP_OP]
	str r3, [r2, #MX51_PLL_DP_HFS_OP]

	str r4, [r2, #MX51_PLL_DP_MFD]
	str r4, [r2, #MX51_PLL_DP_HFS_MFD]

	str r5, [r2, #MX51_PLL_DP_MFN]
	str r5, [r2, #MX51_PLL_DP_HFS_MFN]

	ldr r1, =0x00001232
	str r1, [r2, #MX51_PLL_DP_CTL]
1:	ldr r1, [r2, #MX51_PLL_DP_CTL]
	ands r1, r1, #0x1
	beq 1b
.endm

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#define IMX51_TO_2

.globl board_init_lowlevel
board_init_lowlevel:
	mov     r10, lr

	/* explicitly disable L2 cache */
	mrc 15, 0, r0, c1, c0, 1
	bic r0, r0, #0x2
	mcr 15, 0, r0, c1, c0, 1

	/* reconfigure L2 cache aux control reg */
	mov r0, #0xC0               /* tag RAM */
	add r0, r0, #0x4    /* data RAM */
	orr r0, r0, #(1 << 24)    /* disable write allocate delay */
	orr r0, r0, #(1 << 23)    /* disable write allocate combine */
	orr r0, r0, #(1 << 22)    /* disable write allocate */

	ldr r1, =MX51_IROM_BASE_ADDR
	ldr r3, [r1, #ROM_SI_REV_OFFSET]
	cmp r3, #0x10
	orrls r0, r0, #(1 << 25)    /* disable write combine for TO 2 and lower revs */

	mcr 15, 1, r0, c9, c0, 2

	ldr r0, =MX51_CCM_BASE_ADDR

	/* Gate of clocks to the peripherals first */
	ldr r1, =0x3FFFFFFF
	str r1, [r0, #MX51_CCM_CCGR0]
	ldr r1, =0x0
	str r1, [r0, #MX51_CCM_CCGR1]
	str r1, [r0, #MX51_CCM_CCGR2]
	str r1, [r0, #MX51_CCM_CCGR3]

	ldr r1, =0x00030000
	str r1, [r0, #MX51_CCM_CCGR4]
	ldr r1, =0x00FFF030
	str r1, [r0, #MX51_CCM_CCGR5]
	ldr r1, =0x00000300
	str r1, [r0, #MX51_CCM_CCGR6]

	/* Disable IPU and HSC dividers */
	mov r1, #0x60000
	str r1, [r0, #MX51_CCM_CCDR]

#ifdef IMX51_TO_2
	/* Make sure to switch the DDR away from PLL 1 */
	ldr r1, =0x19239145
	str r1, [r0, #MX51_CCM_CBCDR]
	/* make sure divider effective */
1:	ldr r1, [r0, #MX51_CCM_CDHIPR]
	cmp r1, #0x0
	bne 1b
#endif

	/* Switch ARM to step clock */
	mov r1, #0x4
	str r1, [r0, #MX51_CCM_CCSR]

	mov r3, #MX51_PLL_DP_OP_800
	mov r4, #MX51_PLL_DP_MFD_800
	mov r5, #MX51_PLL_DP_MFN_800
	setup_pll MX51_PLL1_BASE_ADDR

	mov r3, #MX51_PLL_DP_OP_665
	mov r4, #MX51_PLL_DP_MFD_665
	mov r5, #MX51_PLL_DP_MFN_665
	setup_pll MX51_PLL3_BASE_ADDR

	/* Switch peripheral to PLL 3 */
	ldr r1, =0x000010C0
	str r1, [r0, #MX51_CCM_CBCMR]
	ldr r1, =0x13239145
	str r1, [r0, #MX51_CCM_CBCDR]

	mov r3, #MX51_PLL_DP_OP_665
	mov r4, #MX51_PLL_DP_MFD_665
	mov r5, #MX51_PLL_DP_MFN_665
	setup_pll MX51_PLL2_BASE_ADDR

	/* Switch peripheral to PLL2 */
	ldr r1, =0x19239145
	str r1, [r0, #MX51_CCM_CBCDR]
	ldr r1, =0x000020C0
	str r1, [r0, #MX51_CCM_CBCMR]

	mov r3, #MX51_PLL_DP_OP_216
	mov r4, #MX51_PLL_DP_MFD_216
	mov r5, #MX51_PLL_DP_MFN_216
	setup_pll MX51_PLL3_BASE_ADDR

	/* Set the platform clock dividers */
	ldr r2, =MX51_ARM_BASE_ADDR
	ldr r1, =0x00000124
	str r1, [r2, #0x14]

	/* Run TO 3.0 at Full speed, for other TO's wait till we increase VDDGP */
	ldr r1, =MX51_IROM_BASE_ADDR
	ldr r3, [r1, #ROM_SI_REV_OFFSET]
	cmp r3, #0x10
	movls r1, #0x1
	movhi r1, #0
	str r1, [r0, #MX51_CCM_CACRR]

	/* Switch ARM back to PLL 1 */
	mov r1, #0
	str r1, [r0,  #MX51_CCM_CCSR]

        /* setup the rest */
        /* Use lp_apm (24MHz) source for perclk */
#ifdef IMX51_TO_2
        ldr r1, =0x000020C2
        str r1, [r0, #MX51_CCM_CBCMR]
        // ddr clock from PLL 1, all perclk dividers are 1 since using 24MHz
        ldr r1, =0x59239100
        str r1, [r0, #MX51_CCM_CBCDR]
#else
        ldr r1, =0x0000E3C2
        str r1, [r0, #MX51_CCM_CBCMR]
        // emi=ahb, all perclk dividers are 1 since using 24MHz
        // DDR divider=6 to have 665/6=110MHz
        ldr r1, =0x013B9100
        str r1, [r0, #MX51_CCM_CBCDR]
#endif

        /* Restore the default values in the Gate registers */
        ldr r1, =0xFFFFFFFF
        str r1, [r0, #MX51_CCM_CCGR0]
        str r1, [r0, #MX51_CCM_CCGR1]
        str r1, [r0, #MX51_CCM_CCGR2]
        str r1, [r0, #MX51_CCM_CCGR3]
        str r1, [r0, #MX51_CCM_CCGR4]
        str r1, [r0, #MX51_CCM_CCGR5]
        str r1, [r0, #MX51_CCM_CCGR6]

        /* Use PLL 2 for UART's, get 66.5MHz from it */
        ldr r1, =0xA5A2A020
        str r1, [r0, #MX51_CCM_CSCMR1]
        ldr r1, =0x00C30321
        str r1, [r0, #MX51_CCM_CSCDR1]

        /* make sure divider effective */
    1:  ldr r1, [r0, #MX51_CCM_CDHIPR]
        cmp r1, #0x0
        bne 1b

	mov r1, #0x0
	str r1, [r0, #MX51_CCM_CCDR]

	writel(0x1, 0x73fa8074)
	ldr	r0, =0x73f88000
	ldr	r1, [r0]
	orr	r1, #0x40
	str	r1, [r0]

	ldr	r0, =0x73f88004
	ldr	r1, [r0]
	orr	r1, #0x40
	str	r1, [r0]

	mov	pc, r10

