/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 * Copyright (C) 2019 Du Huanpeng <u74147@gmail.com>
 */

#define BOARD_PBL_START start_okud_max9331_boot0

#include <mach/debug_ll.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

        li      ra, 0xbfc20000
        jr      ra
	nop


STOP_WITH_DEBUG_EVENT
