// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2003-2005 Wolfgang Denk, DENX Software Engineering, wd@denx.de.
// SPDX-FileCopyrightText: 2006 Eric Schumann, Phytec Messatechnik GmbH

/* Don't include directly, include <asm/config.h> instead.  */

#ifndef __BOARD_PCM030_H
#define __BOARD_PCM030_H

#include <mach/mpc5xxx.h>

#define CFG_MPC5XXX_CLKIN	33333000 /* ... running at 33.333MHz */

#define CFG_HID0_INIT		HID0_ICE | HID0_ICFI
#define CFG_HID0_FINAL		HID0_ICE

#endif /* __BOARD_PCM030_H */
