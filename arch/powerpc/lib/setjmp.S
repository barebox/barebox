/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * This is a simple version of setjmp and longjmp for the PowerPC.
 * Ian Lance Taylor, Cygnus Support, 9 Feb 1994.
 */

#include <linux/linkage.h>
#include <asm/ppc_asm.tmpl>

ENTRY(setjmp)
	addi	r3,r3,7		# align to 8 byte boundary
	rlwinm	r3,r3,0,0,28
	stw	r1,0(r3)	# offset 0
	stwu	r2,4(r3)	# offset 4
	stwu	r13,4(r3)	# offset 8
	stwu	r14,4(r3)	# offset 12
	stwu	r15,4(r3)	# offset 16
	stwu	r16,4(r3)	# offset 20
	stwu	r17,4(r3)	# offset 24
	stwu	r18,4(r3)	# offset 28
	stwu	r19,4(r3)	# offset 32
	stwu	r20,4(r3)	# offset 36
	stwu	r21,4(r3)	# offset 40
	stwu	r22,4(r3)	# offset 44
	stwu	r23,4(r3)	# offset 48
	stwu	r24,4(r3)	# offset 52
	stwu	r25,4(r3)	# offset 56
	stwu	r26,4(r3)	# offset 60
	stwu	r27,4(r3)	# offset 64
	stwu	r28,4(r3)	# offset 68
	stwu	r29,4(r3)	# offset 72
	stwu	r30,4(r3)	# offset 76
	stwu	r31,4(r3)	# offset 80
	mflr	r4
	stwu	r4,4(r3)	# offset 84
	mfcr	r4
	stwu	r4,4(r3)	# offset 88

	li	r3,0
	blr
END(setjmp)

ENTRY(longjmp)
	addi	r3,r3,7		# align to 8 byte boundary
	rlwinm	r3,r3,0,0,28
	lwz	r1,0(r3)	# offset 0
	lwzu	r2,4(r3)	# offset 4
	lwzu	r13,4(r3)	# offset 8
	lwzu	r14,4(r3)	# offset 12
	lwzu	r15,4(r3)	# offset 16
	lwzu	r16,4(r3)	# offset 20
	lwzu	r17,4(r3)	# offset 24
	lwzu	r18,4(r3)	# offset 28
	lwzu	r19,4(r3)	# offset 32
	lwzu	r20,4(r3)	# offset 36
	lwzu	r21,4(r3)	# offset 40
	lwzu	r22,4(r3)	# offset 44
	lwzu	r23,4(r3)	# offset 48
	lwzu	r24,4(r3)	# offset 52
	lwzu	r25,4(r3)	# offset 56
	lwzu	r26,4(r3)	# offset 60
	lwzu	r27,4(r3)	# offset 64
	lwzu	r28,4(r3)	# offset 68
	lwzu	r29,4(r3)	# offset 72
	lwzu	r30,4(r3)	# offset 76
	lwzu	r31,4(r3)	# offset 80
	lwzu	r5,4(r3)	# offset 84
	mtlr	r5
	lwzu	r5,4(r3)	# offset 88
	mtcrf	255,r5

	mr.	r3,r4
	bclr+	4,2
	li	r3,1
	blr
END(longjmp)

ENTRY(initjmp)
	addi	r3,r3,7		# align to 8 byte boundary
	rlwinm	r3,r3,0,0,28
	stw	r5,0(r3)	# offset 0
	stwu	r4,88(r3)	# offset 88
	li	r3,0
	blr
END(initjmp)
