/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * 64-bit division run-time helpers of the ARM EABI. Both take the dividend
 * in r0:r1 and the divisor in r2:r3 and return the quotient in r0:r1 and
 * the remainder in r2:r3. The division itself is done in ldivmod.c.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>

.macro divmod helper
	push	{r11, lr}
	sub	sp, sp, #16
	add	r12, sp, #8
	str	r12, [sp]		@ pointer to the remainder, 5th argument
	bl	\helper
	ldr	r2, [sp, #8]
	ldr	r3, [sp, #12]
	add	sp, sp, #16
	pop	{r11, pc}
.endm

ENTRY(__aeabi_uldivmod)
	divmod	__udivmoddi4
ENDPROC(__aeabi_uldivmod)

ENTRY(__aeabi_ldivmod)
	divmod	__divmoddi4
ENDPROC(__aeabi_ldivmod)
