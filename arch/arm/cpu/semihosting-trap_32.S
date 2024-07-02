/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2015 Zodiac Inflight Innovations */

/*
 * semihosting-trap_32.S -- Assembly code needed to make a semihosting call
 *
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 */

#include <linux/linkage.h>
#include <asm/unified.h>

.section .text.semihosting_trap
ENTRY(semihosting_trap)
	@ In supervisor mode SVC would clobber LR
	push	{lr}
	ARM(	svc	#0x123456	)
	THUMB(	svc	#0xAB		)
	pop	{pc}
ENDPROC(semihosting_trap)
