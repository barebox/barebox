#ifndef __MACH_K3_DEBUG_LL_H__
#define __MACH_K3_DEBUG_LL_H__
#include <io.h>

#define AM62X_UART_UART0_BASE	0x02800000
#define AM62X_UART_UART1_BASE	0x02810000
#define AM62X_UART_UART2_BASE	0x02820000
#define AM62X_UART_UART3_BASE	0x02830000
#define AM62X_UART_UART4_BASE	0x02840000
#define AM62X_UART_UART5_BASE	0x02850000
#define AM62X_UART_UART6_BASE	0x02860000

#if defined CONFIG_DEBUG_AM62X_UART
#define K3_DEBUG_SOC AM62X_UART

#define __K3_UART_BASE(soc, num) soc##_UART##num##_BASE
#define K3_UART_BASE(soc, num) __K3_UART_BASE(soc, num)

static inline uint8_t debug_ll_read_reg(int reg)
{
	void __iomem *base = (void *)K3_UART_BASE(K3_DEBUG_SOC,
					CONFIG_DEBUG_K3_UART_PORT);

	return readb(base + (reg << 2));
}

static inline void debug_ll_write_reg(int reg, uint8_t val)
{
	void __iomem *base = (void *)K3_UART_BASE(K3_DEBUG_SOC,
					CONFIG_DEBUG_K3_UART_PORT);

	writeb(val, base + (reg << 2));
}

#include <debug_ll/ns16550.h>

static inline void debug_ll_init(void)
{
	/* already configured */
}

static inline void PUTC_LL(int c)
{
	debug_ll_ns16550_putc(c);
}

#endif

#endif /* __MACH_K3_DEBUG_LL_H__ */
