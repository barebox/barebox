/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_black_swift

#include <mach/debug_ll.h>
#include <asm/pbl_macros.h>
#include <asm/asm.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>
#include <linux/sizes.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	ar9331_pbl_generic_start

ENTRY_FUNCTION_END(BOARD_PBL_START, black_swift, SZ_64M)
