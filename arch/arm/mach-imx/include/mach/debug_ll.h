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
#include <mach/imx50-regs.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>
#include <mach/imx6-regs.h>
#include <mach/imx7-regs.h>
#include <mach/vf610-regs.h>

#include <serial/imx-uart.h>
#include <serial/lpuart.h>

#ifdef CONFIG_DEBUG_LL

#define __IMX_UART_BASE(soc, num) soc##_UART##num##_BASE_ADDR
#define IMX_UART_BASE(soc, num) __IMX_UART_BASE(soc, num)

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
#elif defined CONFIG_DEBUG_IMX50_UART
#define IMX_DEBUG_SOC MX50
#elif defined CONFIG_DEBUG_IMX51_UART
#define IMX_DEBUG_SOC MX51
#elif defined CONFIG_DEBUG_IMX53_UART
#define IMX_DEBUG_SOC MX53
#elif defined CONFIG_DEBUG_IMX6Q_UART
#define IMX_DEBUG_SOC MX6
#elif defined CONFIG_DEBUG_IMX7D_UART
#define IMX_DEBUG_SOC MX7
#elif defined CONFIG_DEBUG_VF610_UART
#define IMX_DEBUG_SOC VF610
#else
#error "unknown i.MX debug uart soc type"
#endif

static inline void imx50_uart_setup_ll(void)
{
	void *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC, CONFIG_DEBUG_IMX_UART_PORT));

	imx50_uart_setup(base);
}

static inline void imx51_uart_setup_ll(void)
{
	void *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC, CONFIG_DEBUG_IMX_UART_PORT));

	imx51_uart_setup(base);
}

static inline void imx53_uart_setup_ll(void)
{
	void *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC, CONFIG_DEBUG_IMX_UART_PORT));

	imx53_uart_setup(base);
}

static inline void imx6_uart_setup_ll(void)
{
	void *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC, CONFIG_DEBUG_IMX_UART_PORT));

	imx6_uart_setup(base);
}

static inline void imx7_uart_setup_ll(void)
{
	void *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC, CONFIG_DEBUG_IMX_UART_PORT));

	imx7_uart_setup(base);
}

static inline void vf610_uart_setup_ll(void)
{
	void *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC, CONFIG_DEBUG_IMX_UART_PORT));

	lpuart_setup(base, 66000000);
}

static inline void PUTC_LL(int c)
{
	void __iomem *base = IOMEM(IMX_UART_BASE(IMX_DEBUG_SOC,
						 CONFIG_DEBUG_IMX_UART_PORT));

	if (!base)
		return;

	if (IS_ENABLED(CONFIG_DEBUG_VF610_UART))
		lpuart_putc(base, c);
	else
		imx_uart_putc(base, c);
}

#else

static inline void imx50_uart_setup_ll(void) {}
static inline void imx51_uart_setup_ll(void) {}
static inline void imx53_uart_setup_ll(void) {}
static inline void imx6_uart_setup_ll(void)  {}
static inline void imx7_uart_setup_ll(void)  {}
static inline void vf610_uart_setup_ll(void) {}

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

static inline void vf610_ungate_all_peripherals(void)
{
	void __iomem *ccmbase = IOMEM(VF610_CCM_BASE_ADDR);
	int i;

	for (i = 0x40; i <= 0x6c; i += 4)
		writel(0xffffffff, ccmbase + i);
}

#endif /* __MACH_DEBUG_LL_H__ */
