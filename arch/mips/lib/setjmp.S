/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/regdef.h>
#include <asm/asm.h>
#include <linux/linkage.h>

/* int setjmp (jmp_buf);  */
LEAF(setjmp)
	REG_S	ra, (0 * SZREG)(a0)
	REG_S	sp, (1 * SZREG)(a0)
	REG_S	s0, (2 * SZREG)(a0)
	REG_S	s1, (3 * SZREG)(a0)
	REG_S	s2, (4 * SZREG)(a0)
	REG_S	s3, (5 * SZREG)(a0)
	REG_S	s4, (6 * SZREG)(a0)
	REG_S	s5, (7 * SZREG)(a0)
	REG_S	s6, (8 * SZREG)(a0)
	REG_S	s7, (9 * SZREG)(a0)
	REG_S	fp, (10 * SZREG)(a0)
	move	v0, zero
	j	ra
END(setjmp)

/* volatile void longjmp (jmp_buf, int);  */
LEAF(longjmp)
	REG_L	ra, (0 * SZREG)(a0)
	REG_L	sp, (1 * SZREG)(a0)
	REG_L	s0, (2 * SZREG)(a0)
	REG_L	s1, (3 * SZREG)(a0)
	REG_L	s2, (4 * SZREG)(a0)
	REG_L	s3, (5 * SZREG)(a0)
	REG_L	s4, (6 * SZREG)(a0)
	REG_L	s5, (7 * SZREG)(a0)
	REG_L	s6, (8 * SZREG)(a0)
	REG_L	s7, (9 * SZREG)(a0)
	REG_L	fp, (10 * SZREG)(a0)
	bne	a1, zero, 1f
	li	a1, 1
1:
	move	v0, a1
	j	ra
END(longjmp)

/* int initjmp(jmp_buf jmp, void __noreturn (*func)(void), void *stack_top); */
LEAF(initjmp)
	REG_S	a1, (0 * SZREG)(a0)
	REG_S	a2, (1 * SZREG)(a0)
	move	v0, zero
	j	ra
END(initjmp)
