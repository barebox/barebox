#include <linux/linkage.h>
#include <asm/sections.h>

.section .text.setupc

/*
 * setup_c: copy binary to link address, clear bss and
 * continue executing at new address.
 *
 * This function does not return to the address it is
 * called from, but to the same location in the copied
 * binary.
 */
ENTRY(setup_c)
	push	{r4, r5}
	mov	r5, lr
	bl	get_runtime_offset
	subs	r4, r0, #0
	beq	1f			/* skip memcpy if already at correct address */
	ldr	r0,=_text
	ldr	r2,=__bss_start
	sub	r2, r2, r0
	add	r1, r0, r4
	bl	memcpy			/* memcpy(_text, _text + offset, __bss_start - _text) */
1:	ldr	r0, =__bss_start
	mov	r1, #0
	ldr	r2, =__bss_stop
	sub	r2, r2, r0
	bl	memset			/* clear bss */
#ifdef CONFIG_MMU
	bl	arm_early_mmu_cache_flush
#endif
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0	/* flush icache */
	sub	lr, r5, r4		/* adjust return address to new location */
	pop	{r4, r5}
	mov	pc, lr
ENDPROC(setup_c)

/*
 * void relocate_to_adr(unsigned long targetadr)
 *
 * Copy binary to targetadr, relocate code and continue
 * executing at new address.
 */
.section .text.relocate_to_adr
ENTRY(relocate_to_adr)
					/* r0: target address */
	push	{r3, r4, r5, r6, r7, r8}
	mov	r7, lr

	mov	r6, r0

	bl	get_runtime_offset

	mov	r5, r0

	ldr	r8, =_text

	add	r1, r8, r5		/* r1: from address */

	cmp	r1, r6			/* already at correct address? */
	beq	1f			/* yes, skip copy to new address */

	ldr	r2, =__bss_start

	sub	r2, r2, r8		/* r2: size */
	mov	r0, r6			/* r0: target */

	/* adjust return address */
	sub	r7, r7, r1		/* sub address where we are actually running */
	add	r7, r7, r0		/* add address where we are going to run */

	bl	memcpy			/* copy binary */

#ifdef CONFIG_MMU
	bl	arm_early_mmu_cache_flush
#endif
	mov	r0,#0
	mcr	p15, 0, r0, c7, c5, 0	/* flush icache */

	ldr	r0,=1f
	sub	r0, r0, r8
	add	r0, r0, r6
	mov	pc, r0			/* jump to relocated address */
1:
	bl	relocate_to_current_adr	/* relocate binary */

	mov	lr, r7

	pop	{r3, r4, r5, r6, r7, r8}
	mov	pc, lr

ENDPROC(relocate_to_adr)
