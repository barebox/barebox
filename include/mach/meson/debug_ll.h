/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_MESON_DEBUG_LL_H
#define __MACH_MESON_DEBUG_LL_H

#include <linux/io.h>
#include <serial/meson.h>
#include <mach/meson/a9.h>
#include <mach/meson/g12.h>
#include <mach/meson/gx.h>

#if defined(CONFIG_DEBUG_MESON_A9_UART)
#define MESON_DEBUG_UART_BASE	MESON_A9_UART_B_BASE
#elif defined(CONFIG_DEBUG_MESON_GX_UART)
#define MESON_DEBUG_UART_BASE	MESON_GX_UART_AO_BASE
#else
#define MESON_DEBUG_UART_BASE	MESON_G12_UART_AO_BASE
#endif

static inline void PUTC_LL(int c)
{
	meson_uart_putc(IOMEM(MESON_DEBUG_UART_BASE), c);
}

#endif /* __MACH_MESON_DEBUG_LL_H */
