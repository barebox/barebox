/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_DEBUG_LL_H__
#define __ASM_DEBUG_LL_H__

#ifdef CONFIG_DEBUG_QEMU_ARM64_VIRT
#define DEBUG_LL_UART_ADDR		0x9000000
#include <debug_ll/pl011.h>
#endif

#endif
