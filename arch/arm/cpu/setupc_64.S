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
