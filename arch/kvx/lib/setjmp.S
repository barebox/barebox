/* SPDX-License-Identifier: LGPL-2.1 */
/* SPDX-FileCopyrightText: 2021 Jules Maselbas <jmaselbas@kalray.eu>, Kalray Inc. */

#define REG_SIZE 8

#include <linux/linkage.h>

/* jmp_buf layout:
 * [0]  = $ra,  $sp,  $cs,  $r14,
 * [4]  = $r20, $r21, $r22, $r23,
 * [8]  = $r24, $r25, $r26, $r27,
 * [12] = $r28, $r29, $r30, $r31,
 * [16] = $r18, $r19,
 * [18] = $lc,  $le,  $ls,  xxxx
 */

/**
 * int initjmp(jmp_buf jmp, void __noreturn (*func)(void), void *stack_top);
 */
ENTRY(initjmp)
	/* store $ra */
	sd (0 * REG_SIZE)[$r0] = $r1
	;;
	/* store $sp */
	sd (1 * REG_SIZE)[$r0] = $r2
	make $r0 = 0
	ret
	;;
ENDPROC(initjmp)

/**
 * int setjmp(jmp_buf jmp);
 */
ENTRY(setjmp)
	sq (16 * REG_SIZE)[$r0] = $r18r19
	get $r40 = $ra
	copyd $r41 = $sp
	;;
	so (4 * REG_SIZE)[$r0] = $r20r21r22r23
	get $r42 = $cs
	copyd $r43 = $r14
	;;
	so (0 * REG_SIZE)[$r0] = $r40r41r42r43
	get $r40 = $lc
	;;
	so (8 * REG_SIZE)[$r0] = $r24r25r26r27
	get $r41 = $le
	;;
	so (12 * REG_SIZE)[$r0] = $r28r29r30r31
	get $r42 = $ls
	;;
	so (18 * REG_SIZE)[$r0] = $r40r41r42r43
	make $r0 = 0
	ret
	;;
ENDPROC(setjmp)

/**
 * void longjmp(jmp_buf jmp, int ret);
 */
ENTRY(longjmp)
	lo $r40r41r42r43 = (0 * REG_SIZE)[$r0]
	;;
	lo $r44r45r46r47 = (18 * REG_SIZE)[$r0]
	set $ra = $r40
	copyd $sp = $r41
	;;
	lo $r20r21r22r23 = (4 * REG_SIZE)[$r0]
	set $cs = $r42
	copyd $r14 = $r43
	;;
	lo $r24r25r26r27 = (8 * REG_SIZE)[$r0]
	set $lc = $r44
	;;
	lo $r28r29r30r31 = (12 * REG_SIZE)[$r0]
	set $le = $r45
	;;
	lq $r18r19 = (16 * REG_SIZE)[$r0]
	set $ls = $r46
	;;
	/* According to man, if retval is equal to 0, then we should return 1 */
	maxud $r0 = $r1, 1
	ret
	;;
ENDPROC(longjmp)
