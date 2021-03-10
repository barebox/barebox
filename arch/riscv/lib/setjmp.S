// SPDX-License-Identifier: LGPL-2.1-or-later

/* setjmp for RISC-V. */

#include <linux/linkage.h>
#include <asm/asm.h>

ENTRY (setjmp)
	REG_S ra,  0*SZREG(a0)
	REG_S s0,  1*SZREG(a0)
	REG_S s1,  2*SZREG(a0)
	REG_S s2,  3*SZREG(a0)
	REG_S s3,  4*SZREG(a0)
	REG_S s4,  5*SZREG(a0)
	REG_S s5,  6*SZREG(a0)
	REG_S s6,  7*SZREG(a0)
	REG_S s7,  8*SZREG(a0)
	REG_S s8,  9*SZREG(a0)
	REG_S s9, 10*SZREG(a0)
	REG_S s10,11*SZREG(a0)
	REG_S s11,12*SZREG(a0)
	REG_S sp, 13*SZREG(a0)

	li a0, 0
	ret

END (setjmp)

ENTRY (initjmp)
	REG_S a1,  0*SZREG(a0)
	REG_S a2, 13*SZREG(a0)
	li a0, 0
	ret

END (initjmp)
