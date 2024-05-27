/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Written by H. Peter Anvin <hpa@zytor.com>
 * Brought in from Linux v4.4 and modified for U-Boot
 * From Linux arch/um/sys-i386/setjmp.S
 */

#define _REGPARM

#include <linux/linkage.h>
#include <asm-generic/pointer.h>

.section .note.GNU-stack,"",%progbits

.text
.align 8

/*
 * The jmp_buf is assumed to contain the following, in order:
 *	%ebx
 *	%esp
 *	%ebp
 *	%esi
 *	%edi
 *	<return address>
 */

ENTRY(setjmp)

	movl %eax, %edx
	popl %ecx		/* Return address, and adjust the stack */
	xorl %eax, %eax		/* Return value */
	movl %ebx, (%edx)
	movl %esp, 4(%edx)	/* Post-return %esp! */
	pushl %ecx		/* Make the call/return stack happy */
	movl %ebp, 8(%edx)
	movl %esi, 12(%edx)
	movl %edi, 16(%edx)
	movl %ecx, 20(%edx)	/* Return address */
	ret

ENDPROC(setjmp)

ENTRY(longjmp)

	xchgl %eax, %edx
	movl (%edx), %ebx
	movl 4(%edx), %esp
	movl 8(%edx), %ebp
	movl 12(%edx), %esi
	movl 16(%edx), %edi
	jmp *20(%edx)

ENDPROC(longjmp)

ENTRY(initjmp)

	movl %edx, 20(%eax)	/* Return address */
	movl   $0, 8(%edx)	/* Base pointer */
	sub $ASM_SZPTR, %ecx	/* ESP - 4 has to be 16-byte aligned on entry */
	movl %ecx, 4(%eax)	/* Stack top */
	xorl %eax, %eax		/* Return value */
	ret

ENDPROC(initjmp)
