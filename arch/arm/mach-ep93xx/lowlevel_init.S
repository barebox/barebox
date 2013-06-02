/*
 * Low-level initialization for EP93xx
 *
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * Copyright (C) 2006 Dominic Rath <Dominic.Rath@gmx.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <sizes.h>
#include <mach/ep93xx-regs.h>
#include <asm/barebox-arm-head.h>

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
	bl arm_cpu_lowlevel_init

	/* Turn on both LEDs */
	bl red_LED_on
	bl green_LED_on

	/* Configure flash wait states before we switch to the PLL */
	bl flash_cfg

	/* Set up PLL */
	bl pll_cfg

	/* Turn off the Green LED and leave the Red LED on */
	bl green_LED_off

	/* Setup SDRAM */
	bl sdram_cfg

	/* Turn on Green LED, Turn off the Red LED */
	bl green_LED_on
	bl red_LED_off

	/* switch to async mode */
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #0xc0000000
	mcr p15, 0, r0, c1, c0, 0

	/*
	 * FIXME: This is suitable for the edb9301, the
	 * only ep93xx board we have in our defconfigs.
	 * Other boards need different values here.
	 */
	mov	r0, #0x05000000
	mov	r1, #SZ_8M
	mov	r2, #0
	b	barebox_arm_entry
