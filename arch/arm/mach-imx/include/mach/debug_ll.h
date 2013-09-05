#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>
#include <config.h>
#include <mach/imx1-regs.h>
#include <mach/imx21-regs.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx31-regs.h>
#include <mach/imx35-regs.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>
#include <mach/imx6-regs.h>

#ifdef CONFIG_DEBUG_LL

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

#define __IMX_UART_BASE(soc, num) soc##_UART##num##_BASE_ADDR
#define IMX_UART_BASE(soc, num) __IMX_UART_BASE(soc, num)

#define URTX0		0x40		/* Transmitter Register */

#define UCR1		0x80		/* Control Register 1 */
#define UCR1_UARTEN	(1 << 0)	/* UART enabled */

#define USR2		0x98		/* Status Register 2 */
#define USR2_TXDC	(1 << 3)	/* Transmitter complete */

static inline void PUTC_LL(int c)
{
	void __iomem *base = (void *)IMX_UART_BASE(IMX_DEBUG_SOC,
			CONFIG_DEBUG_IMX_UART_PORT);

	if (!base)
		return;

	if (!(readl(base + UCR1) & UCR1_UARTEN))
		return;

	while (!(readl(base + USR2) & USR2_TXDC));

	writel(c, base + URTX0);
}
#endif /* CONFIG_DEBUG_LL */
#endif /* __MACH_DEBUG_LL_H__ */
