/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <asm/asm.h>
#include <asm/regdef.h>

	.text
	.set	noreorder
LEAF(mips_dead_end)
__error:
	b	__error;
	 nop;

	END(mips_dead_end)
