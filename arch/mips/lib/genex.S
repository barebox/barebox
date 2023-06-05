/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>

	.text
	.set	macro
	.set	noat
	.set	noreorder
	.align	5

/* Exception vector */
NESTED(handle_reserved, 0, sp)
	SAVE_ALL
	PTR_LA	k0, barebox_exc_handler
	jal	k0
	 move	a0, sp
	/* will never return here */
	END(handle_reserved)

/* General exception vector */
NESTED(except_vec3_generic, 0, sp)
	.set	noat
	mfc0	k1, CP0_CAUSE
	PTR_LA	k0, exception_handlers
	andi	k1, k1, 0x7c
	PTR_ADDU	k0, k0, k1
	PTR_L	k0, (k0)
	nop
	jr	k0
	 nop
	END(except_vec3_generic)
	.set	at

FEXPORT(ret_from_exception)
	.set	noat
	RESTORE_ALL_AND_RET
	 nop
	.set	at
