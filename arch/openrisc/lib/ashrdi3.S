/*
 * (C) Copyright 2012 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * Extracted from gcc generated assembly.
 *
 * Extended precision shifts.
 *
 * R3/R4 (MSW, LSW) has 64 bit value
 * R5    has shift count
 * result in R11/R12
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


.globl __ashrdi3

__ashrdi3:
	l.sfeqi r5,0x0			/* if count = 0, go out                   */
	l.bf out

	l.addi r6,r0,0x20		/* r6 = 32                                */
	l.sub r6,r6,r5			/* r6 = 32 - count                        */
	l.sfgtsi r6,0x0			/* if count >= 32                         */
	l.bnf more_than_32		/* branch to more_than_32                 */
	l.nop 0x0


 less_than_32:
	l.sll r6,r3,r6			/* r6 gets the bits moved from MSW to LSW */
	l.srl r4,r4,r5			/* shift LSW                              */
	l.sra r5,r3,r5			/* shift MSW to r5                        */
	l.or r4,r6,r4			/* LSW gets bits shifted from MSW         */
	l.ori r3,r5,0x0			/* r3 = MSW                               */

 out:
	l.ori r11,r3,0x0
	l.jr r9
	l.ori r12,r4,0x0

 more_than_32:
	l.srai r5,r3,0x1f		/* r5 = MSW sign extended                 */
	l.sub r4,r0,r6			/* r4 = -r6, the number of bits above 32  */
	l.sra r4,r3,r4			/* LSW gets bits shifted from MSB         */
	l.j out				/* go out                                 */
	l.ori r3,r5,0x0			/* r3 = MSW                               */
