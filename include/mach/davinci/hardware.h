/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2007 Deep Root Systems, LLC. */

/*
 * Hardware definitions common to all DaVinci family processors
 *
 * Author: Kevin Hilman, Deep Root Systems, LLC
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/memory.h>

/*
 * Before you add anything to this file:
 *
 * This header is for defines common to ALL DaVinci family chips.
 * Anything that is chip specific should go in <chipname>.h,
 * and the chip/board init code should then explicitly include
 * <chipname>.h
 */
/*
 * I/O mapping
 */
#define IO_PHYS				UL(0x01c00000)

#endif /* __ASM_ARCH_HARDWARE_H */
