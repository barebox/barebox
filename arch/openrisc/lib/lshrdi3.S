/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * Extracted from gcc generated assembly.
 *
 * Extended precision shifts.
 *
 * R3/R4 (MSW, LSW) has 64 bit value
 * R5    has shift count
 * result in R11/R12
 *
 */

.globl __lshrdi3

__lshrdi3:
	l.sfeqi r5,0x0
	l.bf out		/* if count = 0, go out                   */

	l.addi r6,r0,0x20	/* r6 = 32                                */
	l.sub r6,r6,r5		/* r6 = 32 - count                        */
	l.sfgtsi r6,0x0		/* if count >= 32                         */
	l.bnf more_than_32	/* branch to more_than_32                 */
	l.nop 0x0

less_than_32:
	l.sll r6,r3,r6		/* r6 gets the bits moved from MSW to LSW */
	l.srl r4,r4,r5		/* shift LSW                              */
	l.srl r3,r3,r5		/* shift MSW                              */
	l.or r4,r6,r4		/* LSW gets bits shifted from MSW         */

 out:
	l.ori r11,r3,0x0
	l.jr r9
	l.ori r12,r4,0x0

more_than_32:
	l.sub r4,r0,r6		/* r4 = -r6, the number of bits above 32  */
	l.srl r4,r3,r4		/* LSW = MSW >> r4                        */
	l.j out			/* go out                                 */
	l.addi r3,r0,0x0	/* MSW = 0                                */
