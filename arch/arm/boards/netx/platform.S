/*
 * Board specific setup info
 *
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <sizes.h>
#include <asm/barebox-arm-head.h>

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
	bl	arm_cpu_lowlevel_init
	mov	r0, #0x80000000
	mov	r1, #SZ_64M
	mov	r2, #0
	b	barebox_arm_entry
