// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru>

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <asm/io.h>
#include <mach/clps711x.h>

static inline void PUTC_LL(char c)
{
	do {
	} while (readl(SYSFLG1) & SYSFLG_UTXFF);

	writew(c, UARTDR1);

	do {
	} while (readl(SYSFLG1) & SYSFLG_UBUSY);
}

#endif
