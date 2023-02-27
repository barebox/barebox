/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __INCLUDE_ARCH_DEBUG_LL_H__
#define __INCLUDE_ARCH_DEBUG_LL_H__

#include <io.h>
#include <soc/fsl/immap_lsch2.h>

#define __LS_UART_BASE(num)	LSCH2_NS16550_COM##num
#define LS_UART_BASE(num) __LS_UART_BASE(num)

static inline uint8_t debug_ll_read_reg(int reg)
{
	void __iomem *base = IOMEM(LS_UART_BASE(CONFIG_DEBUG_LAYERSCAPE_UART_PORT));

	return readb(base + reg);
}

static inline void debug_ll_write_reg(int reg, uint8_t val)
{
	void __iomem *base = IOMEM(LS_UART_BASE(CONFIG_DEBUG_LAYERSCAPE_UART_PORT));

	writeb(val, base + reg);
}

#include <debug_ll/ns16550.h>

static inline void debug_ll_init(void)
{
	uint16_t divisor;

	divisor = debug_ll_ns16550_calc_divisor(300000000);
	debug_ll_ns16550_init(divisor);
}

#endif /* __INCLUDE_ARCH_DEBUG_LL_H__ */
