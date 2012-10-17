#include <linux/linkage.h>

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
	sub	r1, r0, r4
	bl	memcpy			/* memcpy(_text, _text - offset, __bss_start - _text) */
1:	ldr	r0, =__bss_start
	mov	r1, #0
	ldr	r2, =__bss_stop
	sub	r2, r2, r0
	bl	memset			/* clear bss */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0	/* flush icache */
	add	lr, r5, r4		/* adjust return address to new location */
	pop	{r4, r5}
	mov	pc, lr
ENDPROC(setup_c)
