// SPDX-License-Identifier: LGPL-2.1-or-later

/* longjmp, RISC-V version. */

#include <linux/linkage.h>
#include <asm/asm.h>

ENTRY (longjmp)
	REG_L ra,  0*SZREG(a0)
	REG_L s0,  1*SZREG(a0)
	REG_L s1,  2*SZREG(a0)
	REG_L s2,  3*SZREG(a0)
	REG_L s3,  4*SZREG(a0)
	REG_L s4,  5*SZREG(a0)
	REG_L s5,  6*SZREG(a0)
	REG_L s6,  7*SZREG(a0)
	REG_L s7,  8*SZREG(a0)
	REG_L s8,  9*SZREG(a0)
	REG_L s9, 10*SZREG(a0)
	REG_L s10,11*SZREG(a0)
	REG_L s11,12*SZREG(a0)
	REG_L sp, 13*SZREG(a0)

	seqz a0, a1
	add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
	ret

END (longjmp)
