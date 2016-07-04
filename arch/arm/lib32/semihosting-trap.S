/*
 * semihosting-trap.S -- Assembly code needed to make a semihosting call
 *
 * Copyright (c) 2015 Zodiac Inflight Innovations
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
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
