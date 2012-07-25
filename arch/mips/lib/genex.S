#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>

	.text
	.set	macro
	.set	noat
	.set	noreorder
	.align	5

/* Exception vector */
NESTED(handle_reserved, 0, sp)
	la	k0, barebox_exc_handler
	jal	k0
	 move	a0, sp
	/* will never return here */
	END(handle_reserved)

/* General exception vector */
NESTED(except_vec3_generic, 0, sp)
	.set	noat
	mfc0	k1, CP0_CAUSE
	la	k0, exception_handlers
	andi	k1, k1, 0x7c
	addu	k0, k0, k1
	lw	k0, (k0)
	nop
	jr	k0
	 nop
	END(except_vec3_generic)
	.set	at
