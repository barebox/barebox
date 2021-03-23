/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/regdef.h>
#include <asm/asm.h>
#include <linux/linkage.h>

/* int setjmp (jmp_buf);  */
LEAF(setjmp)
	sw	ra, (0 * 4)(a0)
	sw	sp, (1 * 4)(a0)
	sw	s0, (2 * 4)(a0)
	sw	s1, (3 * 4)(a0)
	sw	s2, (4 * 4)(a0)
	sw	s3, (5 * 4)(a0)
	sw	s4, (6 * 4)(a0)
	sw	s5, (7 * 4)(a0)
	sw	s6, (8 * 4)(a0)
	sw	s7, (9 * 4)(a0)
	sw	fp, (10 * 4)(a0)
	move	v0, zero
	j	ra
END(setjmp)

/* volatile void longjmp (jmp_buf, int);  */
LEAF(longjmp)
	lw	ra, (0 * 4)(a0)
	lw	sp, (1 * 4)(a0)
	lw	s0, (2 * 4)(a0)
	lw	s1, (3 * 4)(a0)
	lw	s2, (4 * 4)(a0)
	lw	s3, (5 * 4)(a0)
	lw	s4, (6 * 4)(a0)
	lw	s5, (7 * 4)(a0)
	lw	s6, (8 * 4)(a0)
	lw	s7, (9 * 4)(a0)
	lw	fp, (10 * 4)(a0)
	bne	a1, zero, 1f
	li	a1, 1
1:
	move	v0, a1
	j	ra
END(longjmp)

/* int initjmp(jmp_buf jmp, void __noreturn (*func)(void), void *stack_top); */
LEAF(initjmp)
	sw	a1, (0 * 4)(a0)
	sw	a2, (1 * 4)(a0)
	move	v0, zero
	j	ra
END(initjmp)
