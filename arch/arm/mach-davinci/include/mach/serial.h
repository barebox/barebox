/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2007 MontaVista Software, Inc. */

/*
 * DaVinci serial device definitions
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 */
#ifndef __ASM_ARCH_SERIAL_H
#define __ASM_ARCH_SERIAL_H

#include <mach/hardware.h>

#define DAVINCI_UART0_BASE	(IO_PHYS + 0x20000)
#define DAVINCI_UART1_BASE	(IO_PHYS + 0x20400)
#define DAVINCI_UART2_BASE	(IO_PHYS + 0x20800)

#endif /* __ASM_ARCH_SERIAL_H */
