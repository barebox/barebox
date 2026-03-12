/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <asm/sections.h>
#include <asm/assembler.h>

.section .text.setupc

/*
 * setup_c: clear bss if not yet done
 */
ENTRY(setup_c)
	push	{r4, lr}
	ldr	r0, =bss_cleared
	ldr	r1, [r0]
	cmp	r1, #0
	bne	1f			/* skip if already done */
	ldr	r0, =__bss_start
	mov	r1, #0
	ldr	r2, =__bss_stop
	sub	r2, r2, r0
	bl	__memset		/* clear bss */
	ldr	r0, =bss_cleared
	mov	r1, #1
	str	r1, [r0]		/* mark bss cleared */
1:	pop	{r4, pc}
ENDPROC(setup_c)

.section .data.bss_cleared
.balign 4
bss_cleared:
	.word 	0

/*
 * void relocate_to_adr(unsigned long targetadr)
 *
 * Copy binary to targetadr, relocate code and continue
 * executing at new address.
 */
.section .text.relocate_to_adr
#ifdef __PBL__
ENTRY(relocate_to_adr_full)
	ldr	r2, =__image_end	/* include piggy data */
	b	1f
#endif

ENTRY(relocate_to_adr)
	ldr	r2, =__bss_stop		/* skip piggy data */
	b	1f

1:
	push	{r3, r4, r5, r6, r7, r8}
	mov	r7, lr

	mov	r6, r0			/* r0: target address */

	bl	get_runtime_offset

	mov	r5, r0

	ldr	r8, =_text

	add	r1, r8, r5		/* r1: from address */

	cmp	r1, r6			/* already at correct address? */
	beq	1f			/* yes, skip copy to new address */

	sub	r2, r2, r8		/* r2: size */
	mov	r0, r6			/* r0: target */

	/* adjust return address */
	sub	r7, r7, r1		/* sub address where we are actually running */
	add	r7, r7, r0		/* add address where we are going to run */

	bl	__memcpy			/* copy binary */

	bl	sync_caches_for_execution

	ldr	r0,=1f
	sub	r0, r0, r8
	add	r0, r0, r6
	ret	r0			/* jump to relocated address */
1:
	bl	relocate_to_current_adr	/* relocate binary */

	mov	lr, r7

	pop	{r3, r4, r5, r6, r7, r8}
	ret	lr

ENDPROC(relocate_to_adr)
