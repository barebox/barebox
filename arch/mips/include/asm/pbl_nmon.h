/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * nano-monitor for MIPS CPU
 *
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <mach/debug_ll.h>

#define CODE_ESC	0x1b

/*
 * Delay slot warning!
 *
 * NMON was made with code portability in mind.
 * So it uses '.set reorder' directives allowing
 * assembler to insert necessary 'nop' instructions
 * into delay slots (after branch instruction) and
 * into load delay slot (after memory load instruction
 * on very old R2000/R3000 processors).
 */

	.macro	nmon_outs msg
	.set	push
	.set	reorder

	ADR	a1, \msg, t1

	bal	_nmon_outs

	.set	pop
	.endm

/*
 * output a 32-bit value in hex
 */
.macro debug_ll_outhexw
#ifdef CONFIG_DEBUG_LL
	.set	push
	.set	reorder

	move	t6, a0
	li	t5, 32

202:
	addi	t5, t5, -4
	srlv	a0, t6, t5

	/* output one hex digit */
	andi	a0, a0, 15
	blt	a0, 10, 203f

	addi	a0, a0, ('a' - '9' - 1)

203:
	addi	a0, a0, '0'

	debug_ll_outc_a0

	bgtz	t5, 202b

	.set	pop
#endif /* CONFIG_DEBUG_LL */
.endm

	.macro	mips_nmon
	.set	push
	.set	reorder

#ifdef CONFIG_NMON
#ifdef CONFIG_NMON_USER_START

#if CONFIG_NMON_USER_START_DELAY < 1
#error CONFIG_NMON_USER_START_DELAY must be >= 1!
#endif

	nmon_outs	msg_nmon_press_any_key

	li	s0, CONFIG_NMON_USER_START_DELAY
	move	s1, s0

1:
	li	a0, '.'
	bal	_nmon_outc_a0
	addi	s1, s1, -1
	bnez	s1, 1b

	move	s1, s0

nmon_wait_user:
	pbl_sleep s2, CONFIG_NMON_1S_DELAY

	nmon_outs	msg_bsp

	debug_ll_tstc

	bnez	v0, 3f

	addi	s1, s1, -1
	bnez	s1, nmon_wait_user

	nmon_outs	msg_skipping_nmon

	b	nmon_exit

msg_nmon_press_any_key:
	.asciz "\r\npress any key to start nmon\r\n"

	.align	4
3:
	/* get received char from ns16550's buffer */
	debug_ll_getc
#endif /* CONFIG_NMON_USER_START */

nmon_main_help:
#ifdef CONFIG_NMON_HELP
	nmon_outs	msg_nmon_help
#endif /* CONFIG_NMON_HELP */

nmon_main:
	nmon_outs	msg_prompt

	debug_ll_getc

	/* prepare a0 for debug_ll_outc_a0 */
	move	a0, v0

	li	v1, 'q'
	bne	v0, v1, 3f

	bal	_nmon_outc_a0

	b	nmon_exit

3:
	li	v1, 'd'
	beq	v0, v1, nmon_cmd_d

	li	v1, 'w'
	beq	v0, v1, nmon_cmd_w

	li	v1, 'g'
	beq	v0, v1, nmon_cmd_g

	b	nmon_main_help

nmon_cmd_d:
	bal	_nmon_outc_a0

	li	a0, ' '
	bal	_nmon_outc_a0

	bal	_nmon_gethexw

	nmon_outs	msg_nl

	lw	a0, (v0)
	debug_ll_outhexw

	b	nmon_main

nmon_cmd_w:
	bal	_nmon_outc_a0

	li	a0, ' '
	bal	_nmon_outc_a0
	bal	_nmon_gethexw
	move	s0, v0

	li	a0, ' '
	bal	_nmon_outc_a0
	bal	_nmon_gethexw

	sw	v0, (s0)
	b	nmon_main

nmon_cmd_g:
	bal	_nmon_outc_a0

	li	a0, ' '
	bal	_nmon_outc_a0

	bal	_nmon_gethexw

	nmon_outs	msg_nl

	jal	v0
	b	nmon_main

_nmon_outc_a0:
	debug_ll_outc_a0
	jr	ra

_nmon_outs:
	lbu	a0, 0(a1)
	addi	a1, a1, 1
	beqz	a0, _nmon_jr_ra_exit

	debug_ll_outc_a0

	b	_nmon_outs

_nmon_gethexw:

	li	t3, 8
	li	t2, 0

_get_hex_digit:
	debug_ll_getc

	li	v1, CODE_ESC
	beq	v0, v1, nmon_main

	li	v1, '0'
	bge	v0, v1, 0f
	b	_get_hex_digit

0:
	li	v1, '9'
	ble	v0, v1, 9f

	li	v1, 'f'
	ble	v0, v1, 1f
	b	_get_hex_digit

1:
	li	v1, 'a'
	bge	v0, v1, 8f

	b	_get_hex_digit

8: /* v0 \in {'a', 'b' ... 'f'} */
	sub	a3, v0, v1
	addi	a3, 0xa
	b	0f

9: /* v0 \in {'0', '1' ... '9'} */
	li	a3, '0'
	sub	a3, v0, a3

0:	move	a0, v0
	debug_ll_outc_a0

	sll	t2, t2, 4
	or	t2, t2, a3
	sub	t3, t3, 1

	beqz	t3, 0f

	b	_get_hex_digit

0:
	move	v0, t2

_nmon_jr_ra_exit:
	jr	ra

msg_prompt:
	.asciz "\r\nnmon> "

msg_nl:
	.asciz "\r\n"

msg_bsp:
	.asciz "\b \b"

msg_skipping_nmon:
	.asciz "skipping nmon..."

#ifdef CONFIG_NMON_HELP
msg_nmon_help:
	.ascii "\r\n\r\nnmon commands:\r\n"
	.ascii " q - quit\r\n"
	.ascii " d <addr> - read 32-bit word from <addr>\r\n"
	.ascii " w <addr> <val> - write 32-bit word to <addr>\r\n"
	.ascii " g <addr> - jump to <addr>\r\n"
	.asciz "   use <ESC> key to interrupt current command\r\n"
#endif /* CONFIG_NMON_HELP */

	.align	4

nmon_exit:

	nmon_outs	msg_nl

#endif /* CONFIG_NMON */
	.set	pop
	.endm
