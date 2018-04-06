#include <linux/linkage.h>
#include <asm/sections.h>

.section .text.setupc

/*
 * setup_c: clear bss
 */
ENTRY(setup_c)
	mov	x15, x30
	ldr	x0, =__bss_start
	mov	x1, #0
	ldr	x2, =__bss_stop
	sub	x2, x2, x0
	bl	memset			/* clear bss */
	mov	x30, x15
	ret
ENDPROC(setup_c)

/*
 * void relocate_to_adr(unsigned long targetadr)
 *
 * Copy binary to targetadr, relocate code and continue
 * executing at new address.
 */
.section .text.relocate_to_adr
ENTRY(relocate_to_adr)
					/* x0: target address */

	stp	x19, x20, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!

	mov	x19, x30

	mov	x21, x0

	bl	get_runtime_offset
	mov	x5, x0

	ldr	x0, =_text
	mov	x20, x0

	add	x1, x0, x5		/* x1: from address */

	cmp	x1, x21			/* already at correct address? */
	beq	1f			/* yes, skip copy to new address */

	ldr	x2, =__bss_start

	sub	x2, x2, x0		/* x2: size */
	mov	x0, x21			/* x0: target */

	/* adjust return address */
	sub	x19, x19, x1		/* sub address where we are actually running */
	add	x19, x19, x0		/* add address where we are going to run */

	bl	memcpy			/* copy binary */

#ifdef CONFIG_MMU
	bl	arm_early_mmu_cache_flush
#endif
	mov	x0,#0
	ic	ivau, x0	/* flush icache */

	ldr	x0,=1f
	sub	x0, x0, x20
	add	x0, x0, x21
	br	x0			/* jump to relocated address */
1:
	bl	relocate_to_current_adr	/* relocate binary */

	mov	x30, x19

	ldp	x21, x22, [sp], #16
	ldp	x19, x20, [sp], #16
	ret

ENDPROC(relocate_to_adr)
