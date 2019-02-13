/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_ar9331_spi_trap

#include <mach/debug_ll.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	/* nothing should be done here */

STOP_WITH_DEBUG_EVENT
