/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 * Copyright (C) 2019 Du Huanpeng <u74147@gmail.com>
 */

#define BOARD_PBL_START start_okud_max9331

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

ENTRY_FUNCTION_END(BOARD_PBL_START, ar9331_okud_max9331, SZ_64M)
