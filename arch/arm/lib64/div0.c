// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2002 Wolfgang Denk <wd@denx.de>, DENX Software Engineering

#include <common.h>

extern void __div0(void);

/* Replacement (=dummy) for GNU/Linux division-by zero handler */
void __div0 (void)
{
	panic("division by zero\n");
}
