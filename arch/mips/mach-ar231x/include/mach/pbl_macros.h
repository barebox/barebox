#ifndef __ASM_MACH_AR2312_PBL_MACROS_H
#define __ASM_MACH_AR2312_PBL_MACROS_H

#include <asm/regdef.h>
#include <mach/ar2312_regs.h>

.macro	pbl_ar2312_pll
	.set	push
	.set	noreorder

	mfc0	k0, CP0_STATUS
	li	k1, ST0_NMI
	and	k1, k1, k0
	bnez	k1, pllskip
	 nop

	/* Clear any prior AHB errors by reading both addr registers */
	li	t0, KSEG1 | AR2312_PROCADDR
	lw	zero, 0(t0)
	li	t0, KSEG1 | AR2312_DMAADDR
	lw	zero, 0(t0)

	pbl_sleep	t2, 4000

	li	t0, KSEG1 | AR2312_CLOCKCTL2
	lw	t1, (t0)
	bgez	t1, pllskip	/* upper bit guaranteed non-0 at reset */
	 nop

	/* For Viper 0xbc003064 register has to be programmed with 0x91000 to
	 * get 180Mhz Processor clock
	 * Set /2 clocking and turn OFF AR2312_CLOCKCTL2_STATUS_PLL_BYPASS.
	 * Processor RESETs at this point; the CLOCKCTL registers retain
	 * their new values across the reset.
	 */

	li	t0, KSEG1 | AR2312_CLOCKCTL1
	li	t1, AR2313_CLOCKCTL1_SELECTION
	sw	t1, (t0)

	li	t0, KSEG1 | AR2312_CLOCKCTL2
	li	t1, AR2312_CLOCKCTL2_WANT_RESET
	sw	t1, (t0)	/* reset CPU */
1:	b	1b		/* NOTREACHED */
	 nop
pllskip:

	.set	pop
.endm

.macro	pbl_ar2312_rst_uart0
	.set	push
	.set	noreorder

	li	a0, KSEG1 | AR2312_RESET
	lw	t0, 0(a0)
	and	t0, ~AR2312_RESET_APB
	or	t0, AR2312_RESET_UART0
	sw	t0, 0(a0)
	lw	zero, 0(a0)	/* flush */

	and	t0, ~AR2312_RESET_UART0
	sw	t0, 0(a0)
	lw	zero, 0(a0)	/* flush */

1:	/* Use internal clocking */
	li	a0, KSEG1 | AR2312_CLOCKCTL0
	lw	t0, 0(a0)
	and	t0, ~AR2312_CLOCKCTL_UART0
	sw	t0, 0(a0)

	.set	pop
.endm

.macro	pbl_ar2312_x16_sdram
	.set	push
	.set	noreorder

	li	a0, KSEG1 | AR2312_MEM_CFG0
	li	a1, KSEG1 | AR2312_MEM_CFG1
	li	a2, KSEG1 | AR2312_MEM_REF

	li	a3, MEM_CFG1_E0 | (MEM_CFG1_AC_128 << MEM_CFG1_AC0_S)

	/* Set the I and M bits to issue an SDRAM nop */
	ori	t0, a3, MEM_CFG1_M | MEM_CFG1_I
	sw	t0, 0(a1)	/* AR2312_MEM_CFG1 */

	pbl_sleep	t2, 50

	/* Reset the M bit to issue an SDRAM PRE-ALL */
	ori	t0, a3, MEM_CFG1_I
	sw	t0, 0(a1)	/* AR2312_MEM_CFG1 */
	sync

	/* Generate a refresh every 16 clocks (spec says 10) */
	li	t0, 16		/* very fast refresh for now */
	sw	t0, 0(a2)	/* AR2312_MEM_REF */

	pbl_sleep	t2, 5

	/* Set command write mode, and read SDRAM */
	ori	t0, a3, MEM_CFG1_M
	sw	t0, 0(a1)	/* AR2312_MEM_CFG1 */
	sync

	li	t0, KSEG1 | AR2312_SDRAM0
	or	t0, 0x23000	/* 16bit burst */
	lw	zero, 0(t0)

	/* Program configuration register */
	li	t0, MEM_CFG0_C | MEM_CFG0_C2 | MEM_CFG0_R1 | \
			MEM_CFG0_B0 | MEM_CFG0_X
	sw	t0, 0(a0)	/* AR2312_MEM_CFG0 */
	sync

	li	t0, AR2312_SDRAM_MEMORY_REFRESH_VALUE
	sw	t0, 0(a2)	/* AR2312_MEM_REF */
	sync

	/* Clear I and M and set cfg1 to the normal operational value */
	sw	a3, 0(a1)	/* AR2312_MEM_CFG1 */
	sync

	pbl_sleep	t2, 10

	/* Now we need to set size of RAM to prevent some wired errors.
	 * Since I do not have access to any board with two SDRAM chips, or
	 * any was registered in the wild - we will support only one. */
	/* So, lets find the beef */
	li	a0, KSEG1 | AR2312_MEM_CFG1
	li	a1, KSEG1 | AR2312_SDRAM0
	li	a2, 0xdeadbeef
	li	t0, 0x200000
	li	t1, MEM_CFG1_AC_2

	/* We will write some magic word to the beginning of RAM,
	 * and see if it appears somewhere else. If yes, we made
	 * a travel around the world. */

	/* But first of all save original state of the first RAM word. */
	lw	a3, 0(a1)
	sw	a2, 0(a1)

find_the_beef:
	or	t2, a1, t0
	lw	t3, 0(t2)
	beq	a2, t3, 1f
	 nop
	sll	t0, 1
	add	t1, 1
	/* we should have some limit here. */
	blt	t1, MEM_CFG1_AC_64, find_the_beef
	 nop
	b	make_beefsteak
	 nop

	/* additional paranoid check */
1:
	sw	zero, 0(a1)
	lw	t3, 0(t2)
	bne	zero, t3, find_the_beef
	 nop

make_beefsteak:
	/* create new config for AR2312_MEM_CFG1 and overwrite it */
	sll	t1, MEM_CFG1_AC0_S
	or	t2, t1, MEM_CFG1_E0
	sw	t2, 0(a0)	/* AR2312_MEM_CFG1 */

	/* restore original state of the first RAM word */
	sw	a3, 0(a1)

	.set	pop
.endm

#endif /* __ASM_MACH_AR2312_PBL_MACROS_H */
