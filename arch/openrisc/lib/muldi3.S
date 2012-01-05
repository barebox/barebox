/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * Extracted from gcc generated assembly.
 *
 * Multiply two quads. Hereafter, the illustration of what is going on :
 *
 *                        |    r3    |    r4    |
 *                        |    r5    |    r6    |
 *                         --------------------
 *                        |       r4 * r6       |
 *             |       r3 * r6       |          | +
 *             |       r5 * r4       |          | +
 *  |       r3 * r5       |          |          | +
 *   -------------------------------------------  =
 *                        |    64 bits result   |
 *
 */

.globl __muldi3

__muldi3:
	/* starts with the full 64 bits mul (r4 * r6) */
	l.andi r7,r4,0xffff
	l.srli r8,r4,0x10

	l.andi r11,r6,0xffff
	l.srli r12,r6,0x10

	l.mul r13,r11,r7
	l.mul r11,r11,r8
	l.mul r7,r12,r7

	l.srli r15,r13,0x10
	l.add r7,r7,r15
	l.add r7,r11,r7
	l.sfleu r11,r7
	l.bf no_carry
	l.mul r8,r12,r8

	l.movhi r15,0x1
	l.add r8,r8,r15

no_carry:
	/* Now compute r3 * r6  */
	l.mul r6,r6,r3
	/* and r4 * r5          */
	l.mul r4,r4,r5
	/* finaly previous results and put the result in r11:r12 */
	l.srli r3,r7,0x10
	l.slli r7,r7,0x10
	l.andi r13,r13,0xffff
	l.add r8,r8,r3
	l.add r11,r4,r6
	l.add r12,r7,r13
	l.add r11,r11,r8
	l.jr r9
	l.nop
