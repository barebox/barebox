/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_ROCKCHIP_DEBUG_LL_H__
#define __MACH_ROCKCHIP_DEBUG_LL_H__

#include <common.h>
#include <io.h>
#include <mach/rockchip/rk3188-regs.h>
#include <mach/rockchip/rk3288-regs.h>
#include <mach/rockchip/rk3568-regs.h>
#include <mach/rockchip/rk3588-regs.h>
#include <mach/rockchip/rk3399-regs.h>

#ifdef CONFIG_DEBUG_ROCKCHIP_UART

#ifdef CONFIG_DEBUG_ROCKCHIP_RK3188_UART

#define RK_DEBUG_UART_CLOCK	100000000
#define RK_DEBUG_SOC		RK3188

#elif defined CONFIG_DEBUG_ROCKCHIP_RK3288_UART

#define RK_DEBUG_UART_CLOCK	24000000
#define RK_DEBUG_SOC		RK3288

#elif defined CONFIG_DEBUG_ROCKCHIP_RK3568_UART

#define RK_DEBUG_UART_CLOCK	24000000
#define RK_DEBUG_SOC		RK3568

#elif defined CONFIG_DEBUG_ROCKCHIP_RK3588_UART

#define RK_DEBUG_UART_CLOCK	24000000
#define RK_DEBUG_SOC		RK3588

#elif defined CONFIG_DEBUG_ROCKCHIP_RK3399_UART

#define RK_DEBUG_UART_CLOCK	24000000
#define RK_DEBUG_SOC		RK3399

#endif

#define __RK_UART_BASE(soc, num) soc##_UART##num##_BASE
#define RK_UART_BASE(soc, num) __RK_UART_BASE(soc, num)

static inline uint8_t debug_ll_read_reg(int reg)
{
	void __iomem *base = IOMEM(RK_UART_BASE(RK_DEBUG_SOC,
		CONFIG_DEBUG_ROCKCHIP_UART_PORT));

	return readb(base + (reg << 2));
}

static inline void debug_ll_write_reg(int reg, uint8_t val)
{
	void __iomem *base = IOMEM(RK_UART_BASE(RK_DEBUG_SOC,
		CONFIG_DEBUG_ROCKCHIP_UART_PORT));

	writeb(val, base + (reg << 2));
}

#include <debug_ll/ns16550.h>

static inline void rockchip_debug_ll_init(void)
{
	unsigned int divisor;

	divisor = debug_ll_ns16550_calc_divisor(RK_DEBUG_UART_CLOCK * 2);
	debug_ll_ns16550_init(divisor);
}

static inline void PUTC_LL(int c)
{
	debug_ll_ns16550_putc(c);
}

#else
static inline void rockchip_debug_ll_init(void)
{
}
#endif

#endif /* __MACH_ROCKCHIP_DEBUG_LL_H__ */
