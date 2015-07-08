#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>
#include <config.h>
#include <common.h>
#include <mach/imx1-regs.h>
#include <mach/imx21-regs.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx31-regs.h>
#include <mach/imx35-regs.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>
#include <mach/imx6-regs.h>

#include <serial/imx-uart.h>

#ifdef CONFIG_DEBUG_LL

#define __IMX_UART_BASE(soc, num) soc##_UART##num##_BASE_ADDR
#define IMX_UART_BASE(soc, num) __IMX_UART_BASE(soc, num)

static inline void imx_uart_setup_ll(void __iomem *uartbase,
				     unsigned int refclock)
{
	writel(0x00000000, uartbase + UCR1);

	writel(UCR2_IRTS | UCR2_WS | UCR2_TXEN | UCR2_RXEN | UCR2_SRST,
	       uartbase + UCR2);
	writel(UCR3_DSR | UCR3_DCD | UCR3_RI | UCR3_ADNIMP | UCR3_RXDMUXSEL,
	       uartbase + UCR3);
	writel((0b10 << UFCR_TXTL_SHF) | UFCR_RFDIV1 | (1 << UFCR_RXTL_SHF),
	       uartbase + UFCR);

	writel(baudrate_to_ubir(CONFIG_BAUDRATE),
	       uartbase + UBIR);
	writel(refclock_to_ubmr(refclock),
	       uartbase + UBMR);

	writel(UCR1_UARTEN, uartbase + UCR1);
}

#define __imx_uart_setup_ll(refclock)					\
	do {								\
		imx_uart_setup_ll(IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC,	\
						      CONFIG_DEBUG_IMX_UART_PORT)), \
				  refclock);				\
	} while(0)

#ifdef CONFIG_DEBUG_IMX1_UART
#define IMX_DEBUG_SOC MX1
#elif defined CONFIG_DEBUG_IMX21_UART
#define IMX_DEBUG_SOC MX21
#elif defined CONFIG_DEBUG_IMX25_UART
#define IMX_DEBUG_SOC MX25
#elif defined CONFIG_DEBUG_IMX27_UART
#define IMX_DEBUG_SOC MX27
#elif defined CONFIG_DEBUG_IMX31_UART
#define IMX_DEBUG_SOC MX31
#elif defined CONFIG_DEBUG_IMX35_UART
#define IMX_DEBUG_SOC MX35
#elif defined CONFIG_DEBUG_IMX51_UART
#define IMX_DEBUG_SOC MX51
#elif defined CONFIG_DEBUG_IMX53_UART
#define IMX_DEBUG_SOC MX53
#elif defined CONFIG_DEBUG_IMX6Q_UART
#define IMX_DEBUG_SOC MX6
#else
#error "unknown i.MX debug uart soc type"
#endif

static inline void imx51_uart_setup_ll(void)
{
	__imx_uart_setup_ll(54000000);
}

static inline void imx53_uart_setup_ll(void)
{
	__imx_uart_setup_ll(66666666);
}

static inline void imx6_uart_setup_ll(void)
{
	__imx_uart_setup_ll(80000000);
}

static inline void PUTC_LL(int c)
{
	void __iomem *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC,
						 CONFIG_DEBUG_IMX_UART_PORT));

	if (!base)
		return;

	if (!(readl(base + UCR1) & UCR1_UARTEN))
		return;

	while (!(readl(base + USR2) & USR2_TXDC));

	writel(c, base + URTX0);
}
#else

static inline void imx_uart_setup_ll(void __iomem *uartbase,
				     unsigned int refclock)
{
}

static inline void imx51_uart_setup_ll(void) {}
static inline void imx53_uart_setup_ll(void) {}
static inline void imx6_uart_setup_ll(void)  {}

#endif /* CONFIG_DEBUG_LL */

static inline void imx_ungate_all_peripherals(void __iomem *ccmbase)
{
	int i;
	for (i = 0x68; i <= 0x80; i += 4)
		writel(0xffffffff, ccmbase + i);
}

static inline void imx6_ungate_all_peripherals(void)
{
	imx_ungate_all_peripherals(IOMEM(MX6_CCM_BASE_ADDR));
}

static inline void imx51_ungate_all_peripherals(void)
{
	imx_ungate_all_peripherals(IOMEM(MX51_CCM_BASE_ADDR));
}

static inline void imx53_ungate_all_peripherals(void)
{
	imx_ungate_all_peripherals(IOMEM(MX53_CCM_BASE_ADDR));
}

#endif /* __MACH_DEBUG_LL_H__ */
