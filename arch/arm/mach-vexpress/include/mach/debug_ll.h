/*
 * Copyright 2013 Jean-Christophe PLAGNIOL-VILLARD <plagniol@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <linux/amba/serial.h>
#include <io.h>

#define DEBUG_LL_PHYS_BASE		0x10000000
#define DEBUG_LL_PHYS_BASE_RS1		0x1c000000

#ifdef MP
#define DEBUG_LL_UART_ADDR DEBUG_LL_PHYS_BASE
#else
#define DEBUG_LL_UART_ADDR DEBUG_LL_PHYS_BASE_RS1
#endif

#include <asm/debug_ll_pl011.h>

#endif
