/*
 * Copyright 2013 Jean-Christophe PLAGNIOL-VILLARD <plagniol@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __MACH_VEXPRESS_DEBUG_LL_H__
#define   __MACH_VEXPRESS_DEBUG_LL_H__

#ifdef CONFIG_DEBUG_QEMU_ARM32_VIRT
#define DEBUG_LL_UART_ADDR 0x9000000
#else
#define DEBUG_LL_UART_ADDR 0x1c090000
#endif

#include <debug_ll/pl011.h>

#endif /* __MACH_VEXPRESS_DEBUG_LL_H__ */
