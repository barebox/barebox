// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Matthias Kaehlcke <matthias@kaehlcke.net>

#include <common.h>

/* delay execution before timers are initialized */
static inline void early_udelay(uint32_t usecs)
{
	/* loop takes 4 cycles at 5.0ns (fastest case, running at 200MHz) */
	register uint32_t loops = usecs * (1000 / 20);

	__asm__ volatile ("1:\n"
			"subs %0, %1, #1\n"
			"bne 1b":"=r" (loops):"0" (loops));
}
