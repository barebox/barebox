/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

	.text
	.set	macro
	.set	noat
	.set	noreorder
	.align	5

NESTED(exception_vec, 0, sp)
	j	handle_reserved
	 nop
EXPORT(exception_vec_end)
END(exception_vec)

handle_reserved:
	SAVE_ALL
	PTR_LA	k0, barebox_exc_handler
	j	k0
	 move	a0, sp

FEXPORT(ret_from_exception)
	.set	noat
	RESTORE_ALL_AND_RET
	.set	at
