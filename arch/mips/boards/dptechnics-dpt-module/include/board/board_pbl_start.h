/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	ar9331_pbl_generic_start

	.set	pop
	.endm
