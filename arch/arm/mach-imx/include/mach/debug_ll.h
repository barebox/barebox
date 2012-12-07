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

/* #define IMX_DEBUG_LL_UART_BASE MXxy_UARTx_BASE_ADDR */

#ifndef IMX_DEBUG_LL_UART_BASE
#warning define IMX_DEBUG_LL_UART_BASE properly for debug_ll
#define IMX_DEBUG_LL_UART_BASE 0
#endif

#define URTX0		0x40		/* Transmitter Register */

#define UCR1		0x80		/* Control Register 1 */
#define UCR1_UARTEN	(1 << 0)	/* UART enabled */

#define USR2		0x98		/* Status Register 2 */
#define USR2_TXDC	(1 << 3)	/* Transmitter complete */

static inline void PUTC_LL(int c)
{
	void __iomem *base = (void *)IMX_DEBUG_LL_UART_BASE;

	if (!base)
		return;

	if (!(readl(base + UCR1) & UCR1_UARTEN))
		return;

	while (!(readl(base + USR2) & USR2_TXDC));

	writel(c, base + URTX0);
}

#endif /* __MACH_DEBUG_LL_H__ */
