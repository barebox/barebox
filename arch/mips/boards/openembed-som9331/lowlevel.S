/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_openembed_som9331_board

#include <mach/debug_ll.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>
#include <linux/sizes.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	ar9331_pbl_generic_start
	/* swap PHY4 and PHY0 */
	pbl_reg_writel 0x190, 0xb8070000

ENTRY_FUNCTION_END(BOARD_PBL_START, ar9331_openembed_som9331_board, SZ_64M)
