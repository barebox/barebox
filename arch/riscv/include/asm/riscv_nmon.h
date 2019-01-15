/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * nano-monitor for RISC-V CPU
 *
 * Copyright (C) 2016, 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_RISCV_NMON_H
#define __ASM_RISCV_NMON_H

#define CODE_ESC	0x1b

.macro nmon_outs msg

	la	a1, \msg

	jal	_nmon_outs

.endm

/*
 * output a 32-bit value in hex
 */
.macro debug_ll_outhexw
#ifdef CONFIG_DEBUG_LL
	move	t6, a0
	li	t5, 32

202:
	addi	t5, t5, -4
	srl	a0, t6, t5

	/* output one hex digit */
	andi	a0, a0, 15
	li	t4, 10
	blt	a0, t4, 203f

	addi	a0, a0, ('a' - '9' - 1)

203:
	addi	a0, a0, '0'

	debug_ll_outc_a0

	li	t4, 1
	bge	t5, t4, 202b

#endif /* CONFIG_DEBUG_LL */
.endm

.macro riscv_nmon

#ifdef CONFIG_NMON

nmon_main_help:
#ifdef CONFIG_NMON_HELP
	nmon_outs	msg_nmon_help
#endif /* CONFIG_NMON_HELP */

nmon_main:
	nmon_outs	msg_prompt

	debug_ll_getc

	li	a0, 'q'
	bne	s0, a0, 3f

	jal	_nmon_outc_a0

	j	nmon_exit

3:
	li	a0, 'd'
	beq	s0, a0, nmon_cmd_d

	li	a0, 'w'
	beq	s0, a0, nmon_cmd_w

	li	a0, 'g'
	beq	s0, a0, nmon_cmd_g

	j	nmon_main_help

nmon_cmd_d:
	jal	_nmon_outc_a0

	li	a0, ' '
	jal	_nmon_outc_a0

	jal	_nmon_gethexw

	nmon_outs	msg_nl

	lw	a0, (s0)
	debug_ll_outhexw

	j	nmon_main

nmon_cmd_w:
	jal	_nmon_outc_a0

	li	a0, ' '
	jal	_nmon_outc_a0

	jal	_nmon_gethexw
	move	s2, s0

	li	a0, ' '
	jal	_nmon_outc_a0
	jal	_nmon_gethexw

	sw	s0, 0(s2)
	j	nmon_main

nmon_cmd_g:
	jal	_nmon_outc_a0

	li	a0, ' '
	jal	_nmon_outc_a0

	jal	_nmon_gethexw
	move	s2, s0

	nmon_outs	msg_nl

	jalr	s2
	j	nmon_main

_nmon_outc_a0:
	debug_ll_outc_a0
	jr	ra

_nmon_outs:

	lb	a0, 0(a1)
	addi	a1, a1, 1
	beqz	a0, _nmon_jr_ra_exit

	debug_ll_outc_a0

	j	_nmon_outs

_nmon_gethexw:

	li	t3, 8
	li	t2, 0

_get_hex_digit:
	debug_ll_getc

	li	s1, CODE_ESC
	beq	s0, s1, nmon_main

	li	s1, '0'
	bge	s0, s1, 0f
	j	_get_hex_digit

0:
	li	s1, '9'
	ble	s0, s1, 9f

	li	s1, 'f'
	ble	s0, s1, 1f
	j	_get_hex_digit

1:
	li	s1, 'a'
	bge	s0, s1, 8f

	j	_get_hex_digit

8: /* s0 \in {'a', 'b' ... 'f'} */
	sub	a3, s0, s1
	addi	a3, a3, 0xa
	j	0f

9: /* s0 \in {'0', '1' ... '9'} */
	li	a3, '0'
	sub	a3, s0, a3

0:	move	a0, s0
	debug_ll_outc_a0

	sll	t2, t2, 4
	or	t2, t2, a3
	li	t0, 1
	sub	t3, t3, t0

	beqz	t3, 0f

	j	_get_hex_digit

0:
	move	s0, t2

_nmon_jr_ra_exit:
	jr	ra

msg_prompt:
	.asciz "\r\nnmon> "

msg_nl:
	.asciz "\r\n"

msg_bsp:
	.asciz "\b \b"

#ifdef CONFIG_NMON_HELP
msg_nmon_help:
	.ascii "\r\n\r\nnmon commands:\r\n"
	.ascii " q - quit\r\n"
	.ascii " d <addr> - read 32-bit word from <addr>\r\n"
	.ascii " w <addr> <val> - write 32-bit word to <addr>\r\n"
	.ascii " g <addr> - jump to <addr>\r\n"
	.asciz "   use <ESC> key to interrupt current command\r\n"
#endif /* CONFIG_NMON_HELP */

	.align 2
nmon_exit:
	nmon_outs	msg_nl

#endif /* CONFIG_NMON */

.endm

#endif /* __ASM_RISCV_NMON_H */
